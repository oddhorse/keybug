#include <Arduino.h>
#include <Adafruit_DotStar.h>

#include "hid/hid.h"
#include "ble/ble.h"

// dotstar onboard
Adafruit_DotStar pixel(1, PIN_DOTSTAR_DATA, PIN_DOTSTAR_CLOCK, DOTSTAR_BGR);

// the setup function runs once when you press reset or power the board
void setup()
{
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

	Serial.begin(115200);
#ifdef DEV_BUILD

	while (!Serial)
		delay(10); // blocks until serial monitor opens — dev only
#endif

	ble_init();
	hid_init();

	Serial.println("Adafruit TinyUSB HID Composite example");
}

void loop()
{
	ble_task();
	hid_task();
}
