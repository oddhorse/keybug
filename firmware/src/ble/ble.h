#pragma once

// Sets up the ble stack. Call once from setup().
void ble_init();

// Call every loop() iteration.
void ble_task();
