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
        p.ignoresMouseEvents = true                  // clicks pass through
        p.collectionBehavior = [.canJoinAllSpaces, .fullScreenAuxiliary, .stationary, .ignoresCycle]
        p.contentView = NSHostingView(rootView: CaptureOverlayView())
        p.orderFrontRegardless()
        panel = p
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
