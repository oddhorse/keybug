# firmware/ — PlatformIO project (ItsyBitsy nRF52840)

Full project context (hardware, BLE frame format, milestones, security model) lives in
[`../AGENTS.md`](../AGENTS.md) — read that first. This file only holds what's specific to
working in this folder.

## Current state

- `platformio.ini` targets `env:adafruit_itsybitsy_nrf52840`, `framework = arduino`, and only
  lists `Adafruit DotStar` + `Wire` under `lib_deps`. The Adafruit nRF52 Bluefruit BLE stack and
  Adafruit TinyUSB Library are **not yet added** — needed before Milestone 1 BLE work starts.
- `src/main.cpp` is the stock Adafruit TinyUSB "HID composite" example (onboard button ->
  mouse move + 'a' key + volume-down). It's useful as a working reference for HID report
  descriptors and `Adafruit_USBD_HID` usage, but has nothing to do with KeyBug's BLE frame
  format yet. Don't extend it in place — it's scaffolding proving HID composite enumerates on
  this board, not a base for the real firmware.
- The `src/ble/`, `src/hid/`, `src/msc/`, `src/clip/` subfolders named in the root repo
  structure don't exist yet — that's the intended organization from Milestone 1 onward, not a
  prerequisite.
- Remember: `Wire` must stay explicit in `lib_deps` or TinyUSB headers fail to resolve.

## Collaboration reminder

Same as the root doc: hint/snippet/debug on request, don't autonomously implement whole
milestones.
