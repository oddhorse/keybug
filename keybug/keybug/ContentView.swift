//
//  ContentView.swift
//  keybug
//
//  Created by john on 7/20/26.
//

import SwiftUI

struct ContentView: View {
    @State private var session = SessionController()

    var body: some View {
        VStack(spacing: 8) {
            Image(systemName: session.isCapturing ? "keyboard.fill" : "keyboard")
                .imageScale(.large)
                .foregroundStyle(session.isCapturing ? .green : .secondary)
            Text("Connection: \(String(describing: session.connectionState))")
            Text(session.isCapturing ? "Capturing (Fn+Shift to stop)" : "Idle (Fn+Shift to capture)")
                .foregroundStyle(.secondary)
        }
        .padding()
        .onAppear { session.start() }
    }
}

#Preview {
    ContentView()
}
