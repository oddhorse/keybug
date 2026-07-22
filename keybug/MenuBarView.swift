//
//  MenuBarView.swift
//  keybug
//
//  The MenuBarExtra content: connection status + capture toggle.
//

import SwiftUI

struct MenuBarView: View {
    let session: SessionController

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack(spacing: 6) {
                Circle()
                    .fill(statusColor)
                    .frame(width: 8, height: 8)
                Text(statusText)
                    .font(.subheadline.weight(.medium))
            }

            Divider()

            Button(session.isCapturing ? "Stop Capturing" : "Start Capturing") {
                session.toggleCapture()
            }
            .disabled(session.connectionState != .ready)

            Text("or press Fn + Shift anytime")
                .font(.caption)
                .foregroundStyle(.secondary)

            Divider()

            Button("Quit KeyBug") {
                NSApplication.shared.terminate(nil)
            }
        }
        .padding(12)
        .frame(width: 240)
    }

    private var statusColor: Color {
        switch session.connectionState {
        case .ready:       return session.isCapturing ? .green : .blue
        case .connecting:  return .yellow
        case .scanning:    return .orange
        default:           return .secondary
        }
    }

    private var statusText: String {
        switch session.connectionState {
        case .idle:         return "Bluetooth off"
        case .scanning:     return "Scanning for board…"
        case .connecting:   return "Connecting…"
        case .ready:        return session.isCapturing ? "Capturing" : "Connected"
        case .disconnected: return "Disconnected"
        }
    }
}
