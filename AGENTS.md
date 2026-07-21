# KeyBug — Wireless KVM Multitool

A USB peripheral board that connects to a host computer and presents itself as a standard
keyboard, mouse, and flash drive. The operator controls it wirelessly from their MacBook over
BLE. No software installation is required on the host computer — it uses only standard USB
device classes that every OS already supports.

Primary use case: a developer or technician who needs to control multiple computers without
carrying a separate keyboard and mouse, or who needs to quickly transfer files or clipboard
content to a machine they're working on. This includes bootloader/BIOS/UEFI troubleshooting —
the board must work as keyboard and mouse before any OS (and thus any full HID driver stack)
has loaded, which constrains the USB HID descriptor design (see Feature 1).

## Collaboration mode — read this first

The human is doing the implementation. Default to hints, code snippets, targeted debugging,
and answering specific questions — not writing whole features unprompted. Don't take
initiative to implement a milestone end-to-end just because it's next in the build order.
If a request is ambiguous between "explain/help" and "just do it," ask, or lean toward the
smaller/explain-first option. This applies regardless of which agent/tool is reading this file.

## Current state (update as milestones land)

- **Milestone 0 (BLE transport validation): done.** **Milestone 1 (full input relay): mostly
  working end-to-end, still hacky.**
  - Firmware: `firmware/src/ble/ble.cpp` brings up Bluefruit's `BLEUart` (Adafruit's Nordic
    UART Service wrapper — standard NUS UUIDs, no need to hand-roll the GATT service). Advertises
    as `"keybug!"`. Decodes the 7-byte binary frame format (see Frame Format below — this grew
    from the originally-planned 5 bytes to fit a second `i16` for mouse y-delta) and pushes each
    frame onto a small ring buffer (`firmware/src/hid/hid.cpp`, `QUEUE_SIZE 16`) so `hid_task()`
    can drain it independent of the BLE receive callback.
  - `firmware/src/hid/hid.cpp` has the TinyUSB HID composite device (keyboard + mouse +
    consumer control) fully wired for **keyboard** (6-key rollover state tracked in
    `held_keys`/`held_modifiers`) and **mouse move**. Mouse button and scroll frames are sent by
    the Python client but `process_frame()` doesn't handle `EVT_MOUSE_BTN`/`EVT_SCROLL` yet —
    that dispatch still needs to be added.
  - `firmware/platformio.ini` only lists `Adafruit DotStar` + `Wire` in `lib_deps` — this is
    correct as-is. Bluefruit/TinyUSB/LittleFS come bundled with the Adafruit nRF52 Arduino core
    (selected via `platform = nordicnrf52` / `framework = arduino`), they're not separate
    PlatformIO lib deps.
  - `macos-python/main.py` (folder renamed from `macos/`) is no longer a placeholder: it's a
    working BLE central + global input listener using `bleak` + `pynput` (not a hand-rolled
    `CGEventTap`, though `pynput` uses Quartz/CGEventTap under the hood on macOS). Captures
    keyboard + mouse via `pynput` with `suppress=True`, serializes to the 7-byte frame, queues
    thread-safely into an `asyncio.Queue`, and writes to the NUS RX characteristic with
    `response=False`. No BLE bonding/allowlist yet — connects to whatever's advertising as
    `"keybug!"`.
  - `keybug/` — a Swift/SwiftUI macOS app (Xcode project, `keybug.xcodeproj`) has been scaffolded
    ahead of schedule for the eventual Milestone 4-5 rewrite. Currently just the default
    `uv`-equivalent template (`keybugApp.swift` + `ContentView.swift`, "Hello, world!") — no
    KeyBug logic ported over yet. See "Open Questions" for why this exists before Milestone 4.
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
│       ├── ble/            # BLE peripheral, NUS uart, frame decode + queue
│       ├── hid/             # USB HID composite device, report construction
│       ├── msc/             # USB MSC callbacks, LittleFS integration (not started)
│       └── clip/            # /clip file watcher, BLE send (not started)
├── macos-python/           # Operator laptop control app, Milestones 0-1 (Python/uv)
│   ├── pyproject.toml
│   └── main.py
├── keybug/                 # Operator laptop control app, Milestones 4-5 (Swift/SwiftUI, Xcode)
│   ├── keybug.xcodeproj
│   └── keybug/
├── helper/                 # Optional host-side clipboard helper binary (Milestone 5, not yet created)
│   ├── windows/
│   ├── macos/
│   └── linux/
└── AGENTS.md
```

The `msc/` and `clip/` subfolders under `firmware/src/` don't exist yet — they're the intended
organization once virtual-drive work starts (Milestone 2+), not a requirement to pre-create.
`ble/` and `hid/` already exist and are where Milestone 0-1 work landed.

## Feature 1: BLE Input Relay

The MacBook captures keyboard and mouse events globally via a low-level input listener. Each
event is serialized into a compact binary frame and sent over BLE to the board, which forwards
it as a standard USB HID report to the host machine. The host machine sees a normal USB
keyboard and mouse — including in BIOS/UEFI setup, bootloaders (GRUB, etc.), and other pre-OS
environments, which only understand USB HID Boot Protocol.

### Laptop side
- Global low-level input listener — implemented via `pynput` (`macos-python/main.py`), which
  itself rides on Quartz/`CGEventTap` on macOS, rather than a hand-rolled event tap
- Listener consumes events so they do not also act on the operator's own machine
  (`suppress=True`)
- Each event immediately serialized into a fixed binary frame and sent over BLE (no batching)
- Mouse: send deltas immediately per callback

### BLE transport
- Nordic UART Service (NUS) UUID pair via Bluefruit's `BLEUart` wrapper — not a hand-rolled
  custom GATT service, the standard NUS characteristics are sufficient
- Write Without Response for all event frames — no ACK round trip, minimizes latency
- Connection parameters requested on connect: `Bluefruit.Periph.setConnInterval(6, 12)` (7.5–15ms
  range); slave latency / supervision timeout not yet explicitly tuned
- BLE bonding / allowlist: **not implemented yet** — board currently accepts a connection from
  any central that finds it advertising as `"keybug!"`

### Frame format (binary, fixed width, 7 bytes)
```
[event_type: u8][code: u8][value: i16][value2: i16][modifiers: u8]
```

| ID | Name | `code` | `value` (i16) | `value2` (i16) | `modifiers` |
| --- | --- | --- | --- | --- | --- |
| `0x01` | keyDown | HID usage code | 0 | 0 | modifier bitmask |
| `0x02` | keyUp | HID usage code | 0 | 0 | modifier bitmask |
| `0x03` | mouseMove | 0 | dx | dy | 0 |
| `0x04` | mouseBtn | button bit | 1 = down, 0 = up | 0 | 0 |
| `0x05` | scroll | 0 | vertical delta | horizontal delta | 0 |
| `0x06` | consumerDown | 0 | 16-bit consumer usage | 0 | 0 |
| `0x07` | consumerUp | 0 | 16-bit consumer usage | 0 | 0 |
| `0x08` | clear | 0 | 0 | 0 | 0 |

Named as `EventType` (`firmware/src/frame.h`) — see that file for the `enum class` + the `Frame`
struct these fields map onto. Notes on the less obvious fields:

- `mouseMove`/`scroll` deltas: kept at `i16` on the wire rather than shrunk to `i8` even though a
  single HID mouse report can only carry `int8_t` deltas (`mouseMove`/`mouseScroll` in
  `Adafruit_USBD_HID`) — the truncation point matters. BLE (7.5-15ms connection interval, one
  frame per `write_gatt_char`, no batching) is the expensive hop; USB HID polls every 2ms
  (`setPollInterval(2)`). So the full-precision delta crosses BLE in one frame, and firmware is
  where the ±127 ceiling gets applied — currently via a single clamp per report
  (`clamp_i8` in `hid.cpp`'s `hid_mouse_move`), **not** by splitting one large delta across
  multiple HID reports over successive `hid_task()` ticks. That splitting/chunking is deferred —
  today a delta beyond ±127 in one frame is clamped (movement capped, not lost via wraparound,
  but also not fully replayed). Revisit if that capping is noticeable on fast swipes. Same ceiling
  will apply to `scroll`'s new horizontal (`value2`/pan) component once that's dispatched.
- `consumerDown`/`consumerUp`: media/volume keys. The usage code rides in `value` (i16) rather
  than `code` (u8) because HID consumer control usage codes are 16-bit — too wide for the 1-byte
  `code` field keyboard/mouse events use.
- `clear`: all-fields-zero "reset everything" signal — e.g. clear all held keys/modifiers/mouse
  buttons in one shot, for whatever gets the board into a known-good state.
- `modifiers`: bitmask — shift/ctrl/alt/cmd/fn (left/right variants each get their own bit — see
  `MOD_LEFT_*`/`MOD_RIGHT_*` in `macos-python/main.py`)

Decoded on the firmware side as a raw 7-byte ring buffer entry (`firmware/src/hid/hid.cpp`), not
a struct cast, but same no-string-parsing/no-dynamic-allocation intent. `mouseBtn` and `scroll`
frames are sent by the laptop but not yet dispatched by `process_frame()` on the firmware side.
`consumerDown`/`consumerUp`/`clear` are newer additions to the spec (`frame.h`'s `EventType`
enum already has them) — nothing sends or handles any of the three yet on either side, and there
is currently no USB HID interface for consumer control at all (dropped when `hid.cpp` split into
separate boot-protocol keyboard/mouse interfaces — see Firmware side notes below). Also worth
noting: `enqueue_frame()`'s keyboard-vs-mouse queue routing (`frame[0] < 3` → keyboard queue,
else → mouse queue) predates event types 6-8 and will silently misroute them once anything
actually sends those — needs revisiting alongside whatever handles consumer control/clear.

### Firmware side
- On BLE receive callback: decode frame, immediately construct and send USB HID report
- **Boot Protocol requirement (pre-OS support):** keyboard and mouse each need their own USB HID
  interface with Boot Protocol enabled (`setBootProtocol`) and no Report ID on either — Boot
  Protocol mode uses a fixed report layout with no ID byte, so a Report-ID-multiplexed composite
  report (current single-interface design, one report per function distinguished by ID) is
  invisible to a BIOS/bootloader's minimal HID driver. This rules out keeping keyboard + mouse on
  one composite HID interface; `hid.cpp` now has separate `usb_keyboard`/`usb_mouse` interfaces
  (split done — consumer control interface was dropped in the process, not yet reinstated).
- Consumer control (media/volume keys) has no Boot Protocol equivalent — pre-OS environments
  never read it, so it can stay on its own interface/report without this constraint.
- USB HID polling: request 2ms interval in HID descriptor
- Keyboard/mouse boot-protocol interfaces + eventual MSC interface (Milestone 2) share a limited
  USB endpoint budget on the nRF52840 TinyUSB port — worth verifying headroom once MSC lands.

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

### Milestone 0 — BLE transport validation (Python test script) — done
- [x] `macos-python/` Python project with `uv`, dependencies: `bleak`, `pynput`
- [x] Script scans for KeyBug board by name (`"keybug!"`), connects
- [x] On connect: send frames over NUS RX characteristic (grew from hardcoded test frames into
      the real input-driven relay below)
- [x] Firmware receives frames, decodes, dispatches to `usb_hid` — verified working on host
- [x] (superseded) — real input capture landed alongside this milestone rather than after it

### Milestone 1 — Full input relay — mostly done, still hacky
- [x] PlatformIO project, Adafruit nRF52 BSP, ItsyBitsy nRF52840 target
- [x] TinyUSB composite device: USB HID keyboard + mouse descriptors (+ consumer control)
- [x] BLE peripheral: NUS characteristic (via `BLEUart`), Write Without Response
- [x] BLE connection parameter negotiation — interval range set (6-12 units); slave latency /
      supervision timeout not explicitly tuned yet
- [ ] BLE address allowlist / bonding — not started, board accepts any central
- [x] macOS listener: `pynput` (not raw `CGEventTap`) + `bleak` BLE central + frame serialization
- [x] End-to-end: keyboard fully working. Mouse move works. Mouse button/scroll frames are sent
      but firmware doesn't dispatch them yet (`process_frame()` gap in `hid.cpp`)

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

- **macOS app stack**: Python (`macos-python/`) for Milestones 0-1, Swift (`keybug/`) for
  Milestones 4-5. The Swift Xcode project has been scaffolded early (default SwiftUI template,
  no logic ported yet) so it's ready to receive the port once Milestone 4 starts — this doesn't
  change the intended build order.
- **WiFi transport**: WiFi-direct preferred (no shared router needed)
- **Drive size**: Report honest 2MB rather than lying about capacity
- **Drive label**: TBD — something recognizable but not alarming to IT
- **Wire lib_dep**: Must always be explicitly listed in `platformio.ini` lib_deps or TinyUSB
  headers won't resolve
