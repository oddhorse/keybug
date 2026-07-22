#include <Arduino.h>
#include <Adafruit_DotStar.h>

#include "hid/hid.h"
#include "ble/ble.h"

// dotstar onboard
Adafruit_DotStar pixel(1, PIN_DOTSTAR_DATA, PIN_DOTSTAR_CLOCK, DOTSTAR_BGR);

// the setup function runs once when you press reset or power the board
void setup()
{
	// USB soft-connect (TinyUSBDevice.begin() inside hid_init()) as early as
	// physically possible. A hot-plugged device competes with a much tighter
	// BIOS detection window than one already attached at power-on (which gets
	// the BIOS's own, more patient POST timeline for free) — every ms of
	// delay before this matters. Everything else in setup() can happen after.
	hid_init();

	pixel.begin();
	pixel.setBrightness(50); // 0-255, keep it low, it's bright

	pixel.setPixelColor(0, pixel.Color(255, 255, 0)); // red
	pixel.show();
	delay(500);
	pixel.setPixelColor(0, pixel.Color(0, 255, 0)); // green
	pixel.show();
	delay(500);
	pixel.setPixelColor(0, pixel.Color(0, 0, 255)); // blue
	pixel.show();

#ifdef DEV_BUILD
	// Serial.begin() calls TinyUSBDevice.addInterface(), which adds a CDC
	// interface to the composite USB descriptor — kept out of prod builds
	// entirely so the device presents as HID-only (testing BIOS compatibility).
	Serial.begin(115200);
	while (!Serial)
		delay(10); // blocks until serial monitor opens — dev only
#endif

	ble_init();

	Serial.println("Adafruit TinyUSB HID Composite example");
}

void loop()
{
	ble_task();
	hid_task();
}
