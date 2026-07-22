//
//  SessionController.swift
//  keybug
//
//  Owns the input tap and the BLE link, wires them together, and centralizes
//  everything that reacts to capture turning on/off (cursor, board LED, and
//  later the overlay + menu bar). The composition root for a control session.
//

import CoreGraphics
import Observation

@Observable
class SessionController {
    /// Mirrors InputCapture.capturing into an observable property for the UI.
    private(set) var isCapturing = false

    // Owned subsystems. @ObservationIgnored: SessionController's own observable
    // surface is just `isCapturing`; connection state is observed through `ble`
    // directly (BLECentral is itself @Observable, so ble.state stays tracked).
    @ObservationIgnored let ble = BLECentral()
    @ObservationIgnored private let capture = InputCapture()
    @ObservationIgnored private let overlay = CaptureOverlay()

    /// Convenience passthrough for the UI.
    var connectionState: ConnectionState { ble.state }

    /// Toggle capture from the UI (mirrors the Fn+Shift chord).
    func toggleCapture() {
        capture.toggleCapture()
    }

    /// Wire the callbacks and start the event tap. Call once, from the main
    /// thread (the tap attaches to the current run loop).
    func start() {
        capture.onFrame = { [weak self] frame in
            self?.ble.send(frame)
        }
        capture.onCaptureChanged = { [weak self] on in
            self?.captureChanged(on)
        }
        capture.start()
    }

    private func captureChanged(_ on: Bool) {
        isCapturing = on

        // Visual "controlling remote" signal.
        overlay.setVisible(on)

        // Hide the local cursor during capture (reliable while app is frontmost;
        // the overlay window will make this dependable). Ref-counted — the on/off
        // toggle keeps hide/show balanced.
        if on {
            CGDisplayHideCursor(CGMainDisplayID())
        } else {
            CGDisplayShowCursor(CGMainDisplayID())
        }

        // Reflect capture state on the board's onboard LED. Dropped if not yet
        // connected — the LED syncs on next toggle once the link is up.
        ble.send(.control(.setCaptureLED, value: on ? 1 : 0))
    }
}
