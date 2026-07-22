# keybug

buggggg

## the point

keyboard and mouse forwarder from a laptop to a physically plugged-in device via a bluetooth connection. control laptop -> bluetooth to device -> usb hid kb+m to recipient

## stack

* platformio none of that arduino ide bullshit
* adafruit itsybitsy nrf52840 board (I LOVE THESE GUYS SOOOO MUCH THEY'RE MY FAV BOARD :3 )
* god's grace
* smidge of claude

## to-do

* [ ] make a "release everything" switch that macos software component fires on quit that tells firmware to let up every key
* [x] build a queue for hid sends, so things don't get potentially lost when the keyboard state is overwritten while usb_hid is not ready
* [ ] finish implementing the event type handling on firmware side for all types
  * [x] 0x01 keyDown
  * [x] 0x02 keyUp
  * [o] 0x03 mouseMove
  * [x] 0x04 mouseBtn
  * [o] 0x05 scroll
  * [ ] 0x06 consumerDown
  * [ ] 0x07 consumerUp
  * [ ] 0x08 clear
* [ ] do the chunking code to split too-large pointer or scroll movements into several hid events on the firmware side
* [ ] hide mac cursor or change its appearance when capturing
* [ ] make mac overlay when capturing

## reference

| ID | Name | `code` | `value` (i16) | `value2` (i16) | `modifiers` |
|---|---|---|---|---|---|
| `0x01` | keyDown | HID usage code | 0 | 0 | modifier bitmask |
| `0x02` | keyUp | HID usage code | 0 | 0 | modifier bitmask |
| `0x03` | mouseMove | 0 | dx | dy | 0 |
| `0x04` | mouseBtn | button bit | 1 = down, 0 = up | 0 | 0 |
| `0x05` | scroll | 0 | vertical delta | horizontal delta | 0 |
| `0x06` | consumerDown | 0 | 16-bit consumer usage | 0 | 0 |
| `0x07` | consumerUp | 0 | 16-bit consumer usage | 0 | 0 |
| `0x08` | clear | 0 | 0 | 0 | 0 |
