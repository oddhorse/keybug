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
 	* [ ] 0x03 mouseMove
 	* [ ] 0x04 mouseBtn
 	* [ ] 0x05 scroll
 	* [ ] 0x06 consumerDown
 	* [ ] 0x07 consumerUp
 	* [ ] 0x08 clear
