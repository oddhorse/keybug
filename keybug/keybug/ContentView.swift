//
//  ContentView.swift
//  keybug
//
//  Created by john on 7/20/26.
//

import SwiftUI

struct ContentView: View {
    @State private var ble = BLECentral()
    
    @State private var capture = InputCapture()

    var body: some View {
        VStack {
            Image(systemName: "globe")
                .imageScale(.large)
                .foregroundStyle(.tint)
            Text("Hello, world!")
            Button("Connect") {
                ble.send(Frame(eventType: .keyDown, code: 0x0B))
                ble.send(Frame(eventType: .keyUp, code: 0x0B))
                ble.send(Frame(eventType: .keyDown, code: 0x0C))
                ble.send(Frame(eventType: .keyUp, code: 0x0C))
            }
        }
        .padding()
        .onAppear {
            capture.onCaptureChanged = { on in
                print("capturing: \(on)")
            }
            capture.onFrame = { frame in
                ble.send(frame)
            }
            capture.start()
        }
    }
}

#Preview {
    ContentView()
}
