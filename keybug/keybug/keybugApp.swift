//
//  keybugApp.swift
//  keybug
//
//  Created by john on 7/20/26.
//

import SwiftUI

@main
struct keybugApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) private var appDelegate

    var body: some Scene {
        MenuBarExtra {
            MenuBarView(session: appDelegate.session)
        } label: {
            Image(systemName: appDelegate.session.isCapturing ? "keyboard.fill" : "keyboard")
        }
        .menuBarExtraStyle(.window)   // rich SwiftUI content instead of a plain menu
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    let session = SessionController()

    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory)   // menu-bar only, no Dock icon
        session.start()                          // start the event tap at launch
    }
}
