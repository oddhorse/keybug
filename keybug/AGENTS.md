# keybug/ — Operator laptop control app (Milestones 4-5, Swift)

Full project context (hardware, BLE frame format, milestones, security model) lives in
[`../AGENTS.md`](../AGENTS.md) — read that first. This file only holds what's specific to
working in this folder.

## What lives here

The future Swift/SwiftUI replacement for the operator laptop control app. `../macos-python/` is
the working Milestone 0-1 implementation (BLE central + global input listener, in Python); this
Xcode project is where that gets ported to Swift for Milestones 4-5 (virtual drive live proxy +
clipboard helper), per "Open Questions" in the root doc.

## Current state

- Freshly scaffolded via Xcode's default "App" template — `keybugApp.swift` + `ContentView.swift`
  are still the stock "Hello, world!" SwiftUI starter. No KeyBug logic (BLE, input capture,
  filesystem proxy) has been ported over yet.
- Created ahead of the milestone order as a placeholder/landing spot — Milestones 0-1 work
  should keep happening in `../macos-python/`, not here, until Milestone 4 actually starts.

## Collaboration reminder

Same as the root doc: hint/snippet/debug on request, don't autonomously implement whole
milestones.
