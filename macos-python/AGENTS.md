# macos-python/ — Operator laptop control app (Milestones 0-1)

Full project context (hardware, BLE frame format, milestones, security model) lives in
[`../AGENTS.md`](../AGENTS.md) — read that first. This file only holds what's specific to
working in this folder.

## What lives here

The macOS-side half of KeyBug's Milestone 0 and 1: a `uv`-managed Python project that is the
BLE central + global input listener described in Feature 1 of the root spec. This is the
Milestone 0-1 implementation; Milestone 4-5 work will move to the separate `../keybug/`
Swift/SwiftUI project (currently just an empty Xcode scaffold) — see "Open Questions" in the
root doc.

## Current state

- `main.py` is a working BLE central + global input listener, not a placeholder anymore.
  - Uses `pynput` (`keyboard.Listener`/`mouse.Listener`, `suppress=True`) for global input
    capture and `bleak` (`BleakScanner` + `BleakClient`) for the BLE central side.
  - Scans for the board by device name (`"keybug!"`), connects, and relays keyboard + mouse
    events as 7-byte frames (`struct.pack("<BBhhB", ...)` — see root `AGENTS.md` → Feature 1 →
    Frame format) over the NUS RX characteristic with `write_gatt_char(..., response=False)`.
  - Input callbacks run on `pynput`'s own thread and enqueue into an `asyncio.Queue` via
    `loop.call_soon_threadsafe`; a single `send_loop` coroutine drains the queue and does the
    BLE writes.
  - Keyboard: full key down/up + modifier tracking (`held_keys`, `modifiers` bitmask), including
    a `Ctrl+C`-triggers-`SIGINT` escape hatch to quit the listener cleanly.
  - Mouse: move deltas (dx, dy) are sent; click and scroll frames are also sent, but the
    firmware doesn't dispatch those event types yet (see root `AGENTS.md` current-state notes).
  - No BLE bonding/allowlist on this side either — connects to whatever advertises as
    `"keybug!"`.
- Milestone 0 (BLE harness) and most of Milestone 1 (real input relay) are done from the laptop
  side. Remaining gaps are mostly firmware-side (mouse button/scroll dispatch, BLE bonding).

## Collaboration reminder

Same as the root doc: hint/snippet/debug on request, don't autonomously implement whole
milestones.
