#pragma once

#include <Arduino.h>

// Sets up the TinyUSB HID composite device (keyboard + mouse + consumer
// control) and the demo button pin. Call once from setup().
void hid_init();

// Polls the button and sends HID reports on a fixed interval. Call every
// loop() iteration.
void hid_task();

void process_keystroke(uint8_t frame_buf[5]);
