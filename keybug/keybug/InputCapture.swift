//
//  InputCapture.swift
//  keybug
//
//  Created by john on 7/21/26.
//

import CoreGraphics
import Foundation

final class InputCapture {
    var onFrame: ((Frame) -> Void)?          // SessionController wires this to ble.send
    var onCaptureChanged: ((Bool) -> Void)?  // notify UI / overlay when toggled

    /// When true, input is suppressed locally and forwarded to the board.
    /// When false, the tap still runs (to watch for the toggle chord) but
    /// events pass through to this Mac normally.
    private(set) var capturing = false

    private var tap: CFMachPort?
    private var runLoopSource: CFRunLoopSource?
    private var modifiers: UInt8 = 0
    private var toggleChordActive = false

    /// Flip to true to log each decoded event to the console.
    static let verboseLogging = false

    /// @autoclosure: when logging is off, the string is never even built.
    private func log(_ message: @autoclosure () -> String) {
        if Self.verboseLogging { print(message()) }
    }

    func start() {
        // Which event types we want. Each CGEventType is a bit index; shift 1 into
        // that position and OR them together to build the mask.
        let mask: CGEventMask =
            (1 << CGEventType.keyDown.rawValue) |
            (1 << CGEventType.keyUp.rawValue) |
            (1 << CGEventType.flagsChanged.rawValue)

        // `refcon` smuggles `self` into the C callback, which can't capture context.
        let refcon = Unmanaged.passUnretained(self).toOpaque()

        guard let tap = CGEvent.tapCreate(
            tap: .cgSessionEventTap,
            place: .headInsertEventTap,
            options: .defaultTap,          // .defaultTap = allowed to modify/swallow events
            eventsOfInterest: mask,
            callback: { _, type, event, refcon in
                let capture = Unmanaged<InputCapture>.fromOpaque(refcon!).takeUnretainedValue()

                // The system disables the tap if our callback is ever too slow;
                // these two events are our cue to turn it back on.
                if type == .tapDisabledByTimeout || type == .tapDisabledByUserInput {
                    if let tap = capture.tap { CGEvent.tapEnable(tap: tap, enable: true) }
                    return Unmanaged.passUnretained(event)
                }

                // handle() decides whether this event is swallowed (nil) or
                // passed through to the local machine.
                let suppress = capture.handle(type: type, event: event)
                return suppress ? nil : Unmanaged.passUnretained(event)
            },
            userInfo: refcon
        ) else {
            // nil almost always means missing Accessibility / Input Monitoring permission.
            print("InputCapture: tapCreate failed — check Accessibility & Input Monitoring")
            return
        }

        self.tap = tap
        runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0)
        CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, .commonModes)
        CGEvent.tapEnable(tap: tap, enable: true)
    }

    func stop() {
        if let tap { CGEvent.tapEnable(tap: tap, enable: false) }
        if let runLoopSource {
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, .commonModes)
        }
        tap = nil
        runLoopSource = nil
    }

    /// Returns true if the event should be suppressed (not delivered to this Mac).
        private func handle(type: CGEventType, event: CGEvent) -> Bool {
            // 1) Toggle chord — watched in BOTH states so it can turn capture on or off.
            if type == .flagsChanged {
                let chord = event.flags.contains(.maskSecondaryFn) && event.flags.contains(.maskShift)
                if chord && !toggleChordActive {
                    toggleChordActive = chord
                    setCapturing(!capturing)
                    return true   // swallow the completing keystroke, either direction
                }
                toggleChordActive = chord
            }

        // 2) Not capturing: let the event reach the local machine untouched.
        guard capturing else { return false }

        // 3) Capturing: decode → onFrame, and suppress locally.
        let keycode = Int(event.getIntegerValueField(.keyboardEventKeycode))

        // Derive the modifier byte straight from the event's flags — ground truth,
        // so it can't desync (the Fn+Shift chord always leaves Shift held at the
        // moment capture turns on, which is what a manual toggle mis-tracks).
        // The low bits are the device-dependent left/right masks (IOKit NX_DEVICE*).
        let raw = event.flags.rawValue
        var mods: UInt8 = 0
        if raw & 0x00000001 != 0 { mods |= MOD_LEFT_CTRL }
        if raw & 0x00002000 != 0 { mods |= MOD_RIGHT_CTRL }
        if raw & 0x00000002 != 0 { mods |= MOD_LEFT_SHIFT }
        if raw & 0x00000004 != 0 { mods |= MOD_RIGHT_SHIFT }
        if raw & 0x00000020 != 0 { mods |= MOD_LEFT_ALT }
        if raw & 0x00000040 != 0 { mods |= MOD_RIGHT_ALT }
        if raw & 0x00000008 != 0 { mods |= MOD_LEFT_GUI }
        if raw & 0x00000010 != 0 { mods |= MOD_RIGHT_GUI }
        let wentDown = (mods & ~modifiers) != 0   // a bit turned on since last time
        let changed = mods != modifiers
        modifiers = mods

        switch type {
        case .keyDown, .keyUp:
            // Unmapped keys (e.g. the globe key, 179) yield nil and are skipped.
            if let hid = VK_TO_HID[keycode] {
                log("KEY \(type == .keyDown ? "down" : "up  ") keycode \(keycode) hid \(String(format: "0x%02X", hid)) mods \(String(format: "0x%02X", modifiers))")
                onFrame?(Frame(eventType: type == .keyDown ? .keyDown : .keyUp,
                               code: hid, modifiers: modifiers))
            } else {
                log("KEY unmapped keycode \(keycode)")
            }
        case .flagsChanged:
            // event_type is irrelevant to the firmware for a code:0 frame (it just
            // copies the modifier byte), but we report down/up for a readable log.
            if changed {
                log("MOD keycode \(keycode) -> mods \(String(format: "0x%02X", modifiers))")
                onFrame?(Frame(eventType: wentDown ? .keyDown : .keyUp, modifiers: modifiers))
            }
        default:
            break   // mouse types land here once added to the mask
        }
        return true
    }

    private func setCapturing(_ on: Bool) {
        // Turning off mid-hold (e.g. Shift is down as part of the toggle chord)
        // would leave those keys stuck down on the remote — clear releases them.
        if !on {
            onFrame?(Frame(eventType: .clear))
        }
        capturing = on
        modifiers = 0
        onCaptureChanged?(on)
    }
}
