//
//  CaptureOverlay.swift
//  keybug
//
//  Borderless, always-on-top window shown while capture is active — the visual
//  "this Mac is driving the remote machine" signal. Click-through and
//  non-activating so it never interferes with input or steals focus.
//

import AppKit
import SwiftUI

@MainActor
final class CaptureOverlay {
    private var panel: NSPanel?

    func setVisible(_ visible: Bool) {
        visible ? show() : hide()
    }

    private func show() {
        guard panel == nil, let screen = NSScreen.main else { return }
        let p = NSPanel(contentRect: screen.frame,
                        styleMask: [.borderless, .nonactivatingPanel],
                        backing: .buffered, defer: false)
        p.level = .screenSaver                       // above normal & fullscreen windows
        p.isOpaque = false
        p.backgroundColor = .clear
        p.hasShadow = false
        // Accept mouse events so the cursor counts as "over our window" across the
        // whole screen — that's what lets CGDisplayHideCursor hide it everywhere,
        // not just over the menu popup. Harmless: the event tap suppresses all
        // mouse input before any window sees it while capturing.
        p.ignoresMouseEvents = false
        p.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary, .stationary, .ignoresCycle]
        p.contentView = NSHostingView(rootView: CaptureOverlayView())
        p.orderFrontRegardless()
        panel = p

        // Become the active app so CGDisplayHideCursor takes effect — as an
        // .accessory (menu-bar) app we're otherwise never frontmost, and cursor
        // hiding only works for the active app. Steals focus for the duration of
        // capture, which is the intent (you're driving the remote, not this Mac).
        NSApp.activate(ignoringOtherApps: true)
    }

    private func hide() {
        panel?.orderOut(nil)
        panel = nil
    }
}

private struct CaptureOverlayView: View {
    var body: some View {
        ZStack {
            Rectangle()
                .strokeBorder(Color.red.opacity(0.9), lineWidth: 4)
            VStack {
                Text("KeyBug — controlling remote  ·  Fn + Shift to release")
                    .font(.caption.weight(.semibold))
                    .foregroundStyle(.white)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 6)
                    .background(Color.red.opacity(0.9), in: Capsule())
                    .padding(.top, 6)
                Spacer()
            }
        }
        .ignoresSafeArea()
    }
}
