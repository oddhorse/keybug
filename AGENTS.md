# KeyBug — Wireless KVM Multitool

A USB peripheral board that connects to a host computer and presents itself as a standard
keyboard, mouse, and flash drive. The operator controls it wirelessly from their MacBook over
BLE. No software installation is required on the host computer — it uses only standard USB
device classes that every OS already supports.

Primary use case: a developer or technician who needs to control multiple computers without
carrying a separate keyboard and mouse, or who needs to quickly transfer files or clipboard
content to a machine they're working on.

## Collaboration mode — read this first

The human is doing the implementation. Default to hints, code snippets, targeted debugging,
and answering specific questions — not writing whole features unprompted. Don't take
initiative to implement a milestone end-to-end just because it's next in the build order.
If a request is ambiguous between "explain/help" and "just do it," ask, or lean toward the
smaller/explain-first option. This applies regardless of which agent/tool is reading this file.

## Current state (update as milestones land)

- **Milestone 0 (BLE transport validation): in progress.**
  - Firmware: `firmware/src/ble/ble.cpp` brings up Bluefruit's `BLEUart` (Adafruit's Nordic
    UART Service wrapper — standard NUS UUIDs, no need to hand-roll the GATT service). Advertises
    as `"keybug!"`. Currently a raw serial-bridge demo: bytes in over BLEUART are treated as
    ASCII and mapped through `ascii_to_hid` (in `ble.cpp`) to keystrokes via
    `hid::send_keystroke`. **Does not yet decode the spec's 5-byte binary frame format** —
    that's still to build.
  - `firmware/src/hid/hid.cpp` has the TinyUSB HID composite device (keyboard + mouse +
    consumer control) and a demo button handler carried over from the stock example, plus
    `send_keystroke()` used by the BLE side.
  - `firmware/platformio.ini` only lists `Adafruit DotStar` + `Wire` in `lib_deps` — this is
    correct as-is. Bluefruit/TinyUSB/LittleFS come bundled with the Adafruit nRF52 Arduino core
    (selected via `platform = nordicnrf52` / `framework = arduino`), they're not separate
    PlatformIO lib deps.
  - `macos/main.py` is still the `uv init` placeholder — scan/connect/send-test-frame script not
    written yet.
- `helper/` (Milestone 5 clipboard helper binary) does not exist yet.

## Hardware

**Board:** Adafruit ItsyBitsy nRF52840
- nRF52840 ARM Cortex-M4 @ 64MHz
- 1MB program flash, 256KB RAM
- 2MB onboard QSPI flash (backs the virtual filesystem)
- Native USB (TinyUSB) — enumerates as composite HID + MSC
- BLE 5.0 radio — acts as BLE peripheral to operator laptop

**Toolchain:** PlatformIO + Adafruit nRF52 Arduino BSP

**Libraries:**
- `adafruit/Adafruit nRF52` — BSP, Bluefruit BLE stack, LittleFS/InternalFileSystem
- `adafruit/Adafruit TinyUSB Library` — composite USB HID + MSC
- `Wire` — must be listed explicitly in lib_deps or PlatformIO build fails

## System Overview

```
[Operator MacBook]
  └─ macOS control app (Python, later Swift)
       ├─ Global input event listener (keyboard + mouse)
       ├─ BLE central — connects to KeyBug board
       ├─ Virtual drive watcher (monitors /clip file writes)
       └─ Laptop-side filesystem proxy (serves block I/O for MSC)
            │
            │  BLE (GATT, NUS-style, Write Without Response)
            │  + WiFi/LAN for bulk file transfer (stretch goal)
            │
[ItsyBitsy nRF52840 — connected to host computer USB]
  ├─ BLE peripheral — connected to operator laptop
  ├─ USB composite device to host:
  │    ├─ HID (keyboard + mouse)
  │    └─ MSC (virtual flash drive, backed by QSPI LittleFS)
  └─ Firmware:
       ├─ Receives BLE frames → emits USB HID reports
       ├─ Watches LittleFS for /clip file writes → sends contents to laptop
       └─ Serves/accepts block-level MSC I/O proxied to laptop

[Host Machine] (any OS, no software install)
  ├─ Sees: USB keyboard + mouse (standard plug and play)
  └─ Sees: USB flash drive (standard plug and play)
```

## Repo Structure

```
keybug/
├── firmware/               # PlatformIO project
│   ├── platformio.ini
│   └── src/
│       ├── main.cpp
│       ├── ble/            # BLE peripheral, GATT service, bonding
│       ├── hid/             # USB HID report construction, descriptors
│       ├── msc/             # USB MSC callbacks, LittleFS integration
│       └── clip/            # /clip file watcher, BLE send
├── macos/                  # Operator laptop control app (Python/uv)
│   ├── pyproject.toml
│   └── main.py
├── helper/                 # Optional host-side clipboard helper binary (Milestone 5, not yet created)
│   ├── windows/
│   ├── macos/
│   └── linux/
└── AGENTS.md
```

The `ble/`, `hid/`, `msc/`, `clip/` subfolders under `firmware/src/` don't exist yet — they're
the intended organization once BLE/MSC work starts (Milestone 1+), not a requirement to
pre-create.

## Feature 1: BLE Input Relay

The MacBook captures keyboard and mouse events globally via a low-level input listener. Each
event is serialized into a compact binary frame and sent over BLE to the board, which forwards
it as a standard USB HID report to the host machine. The host machine sees a normal USB
keyboard and mouse.

### Laptop side
- Global low-level input listener (`CGEventTap` on macOS), requires Accessibility permission
- Listener consumes events so they do not also act on the operator's own machine
- Each event immediately serialized into a fixed binary frame and sent over BLE (no batching)
- Mouse: send deltas immediately per callback

### BLE transport
- Custom GATT service using Nordic UART Service (NUS) UUID pair
- Write Without Response for all event frames — no ACK round trip, minimizes latency
- Connection parameters to request on connect:
  - Interval: 6 units (= 7.5ms, BLE spec minimum, units are 1.25ms steps)
  - Slave latency: 0
  - Supervision timeout: 4000ms
- BLE bonding: allowlist locked to operator laptop's BLE address only

### Frame format (binary, fixed width, 5 bytes)
```
[event_type: u8][code: u8][value: i16][modifiers: u8]
```
- `event_type`: `0x01` key_down, `0x02` key_up, `0x03` mouse_move, `0x04` mouse_button, `0x05` scroll
- `code`: HID usage code (keys) or button index (mouse)
- `value`: signed delta for mouse_move/scroll, 0 for key events
- `modifiers`: bitmask — shift/ctrl/alt/cmd/fn

Decode on firmware side with a static struct cast — no string parsing, no dynamic allocation.

### Firmware side
- On BLE receive callback: decode frame, immediately construct and send USB HID report
- Composite USB device: HID report descriptor covers keyboard, mouse, and consumer control
- USB HID polling: request 2ms interval in HID descriptor

### Latency budget
- BLE interval: ~7.5ms (negotiated)
- Input listener overhead: ~1-2ms
- USB HID host poll: 2-8ms depending on host
- Target end-to-end feel: ~10-20ms

## Feature 2: Virtual Drive + File Proxy

The board presents a USB Mass Storage Class device to the host. The host sees a normal flash
drive. Internally, storage is backed by the board's onboard QSPI flash running LittleFS.

### Two implementation tiers (build in order)

**Tier 1 — Local flash storage**
- Files write to QSPI LittleFS on the board
- No live sync — files persist on board, retrieved later
- Validates MSC enumeration and LittleFS before adding wireless complexity

**Tier 2 — Live proxy**
- Block-level read/write callbacks relay to operator laptop over BLE (small ops) or WiFi/LAN
  (bulk data)
- Laptop-side service translates block I/O to real file operations on a local folder
- BLE alone insufficient for bulk transfer — WiFi/LAN path needed for files > a few KB

### Filesystem access conflict
USB host and firmware cannot write to LittleFS simultaneously. Firmware briefly unmounts from
USB MSC while reading/writing, then remounts. Must complete within USB timeout window.

## Feature 3: Clipboard File Transfer

A file named `clip` on the virtual drive acts as a one-way pipe to the operator's macOS
clipboard. Any shell on any OS can write to it using standard output redirection — no special
syntax, no installed tools.

```
# Windows cmd / PowerShell
ipconfig /all > D:\clip

# bash / zsh / fish
some_command > /Volumes/KEYBUG/clip
```

Works in recovery environments, minimal shells, or any context where writing to a USB drive is
possible.

### Firmware behavior
1. After any write to LittleFS, check if path == `/clip`
2. If yes: read contents, send to laptop via BLE, delete file
3. Laptop receives contents, places them in macOS clipboard via pbcopy/Cocoa pasteboard

### MSC write detection
MSC writes arrive as raw block-level sector writes. Use a dirty flag in the MSC write callback,
then poll LittleFS for `/clip` on main loop tick.

## Feature 4: Clipboard Helper Binary (Optional)

OS clipboard APIs are not accessible from a USB device class. For bidirectional clipboard sync,
a small portable binary is pre-loaded on the virtual drive. The user runs it once from the host
machine. It connects back to the board over BLE or local WiFi and syncs clipboard contents in
both directions.

| Platform | Notes |
|---|---|
| Windows | Standalone .exe, no admin rights needed. User double-clicks once from the drive. |
| macOS | Requires signing + notarization (Apple Developer account, $99/yr) for clean launch. |
| Linux | Static binary + execute permission. X11 clipboard straightforward; Wayland may restrict. |

## Security

- BLE bonding: allowlist locked to operator laptop's BLE address. Board rejects all other
  connection attempts.
- New bonds only accepted when board is in pairing mode (physical button press required).
- Helper binary signed and notarized to avoid triggering endpoint security tools on host
  machines.

## Build Order

### Milestone 0 — BLE transport validation (Python test script)
- [ ] `macos/` Python project with `uv`, dependencies: `bleak`, `pynput`
- [ ] Script scans for KeyBug board by NUS service UUID, connects
- [ ] On connect: send hardcoded test frames (key_down 'a', key_up, mouse delta) over NUS RX
      characteristic
- [ ] Firmware receives frames, decodes, dispatches to `usb_hid` — verify output appears on
      host machine
- [ ] No input capture yet — purely a BLE send/receive test harness

### Milestone 1 — Full input relay
- [ ] PlatformIO project, Adafruit nRF52 BSP, ItsyBitsy nRF52840 target
- [ ] TinyUSB composite device: USB HID keyboard + mouse descriptors
- [ ] BLE peripheral: custom GATT service, NUS characteristic, Write Without Response
- [ ] BLE connection parameter negotiation (7.5ms interval, slave latency 0)
- [ ] BLE address allowlist / bonding
- [ ] macOS listener: CGEventTap, BLE central, frame serialization
- [ ] End-to-end: keyboard and mouse input from MacBook appears on host machine

### Milestone 2 — Virtual drive, Tier 1 (local flash)
- [ ] TinyUSB MSC added to composite device (HID + MSC simultaneously)
- [ ] QSPI flash formatted as LittleFS, mounted and accessible
- [ ] MSC read/write callbacks wired to LittleFS
- [ ] Filesystem access conflict handled (brief unmount on firmware write)
- [ ] End-to-end: host machine sees drive, can read/write files, files persist on board

### Milestone 3 — Clipboard file transfer
- [ ] Write-detection on LittleFS (dirty flag + main loop poll)
- [ ] `/clip` file detection, read, BLE send to laptop, delete
- [ ] Laptop-side: receive clip contents, push to macOS clipboard
- [ ] End-to-end: `echo hello > D:\clip` on host → "hello" in MacBook clipboard

### Milestone 4 — Virtual drive, Tier 2 (live proxy)
- [ ] Laptop-side filesystem proxy service (serves block I/O over BLE/WiFi)
- [ ] Firmware MSC callbacks relay block reads/writes to laptop
- [ ] WiFi/LAN bulk transfer path for large files
- [ ] End-to-end: file written to virtual drive on host appears in laptop folder in real time

### Milestone 5 — Clipboard helper binary
- [ ] Minimal binary that connects to board over BLE or local WiFi
- [ ] Bidirectional clipboard sync loop
- [ ] Windows build (.exe, no installer, no admin)
- [ ] macOS build (signed + notarized)
- [ ] Linux build (static binary)
- [ ] Pre-loaded on virtual drive

## Open Questions

- **macOS app stack**: Python for Milestones 0-1, Swift for Milestones 4-5
- **WiFi transport**: WiFi-direct preferred (no shared router needed)
- **Drive size**: Report honest 2MB rather than lying about capacity
- **Drive label**: TBD — something recognizable but not alarming to IT
- **Wire lib_dep**: Must always be explicitly listed in `platformio.ini` lib_deps or TinyUSB
  headers won't resolve
