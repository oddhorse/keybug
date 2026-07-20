#include "hid.h"

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"

/* Demo button -> HID reports, moved as-is from the stock Adafruit example.
 * Depending on the board, the button pin and its active state (when
 * pressed) are different.
 */

static const int pin = PIN_BUTTON1;
static bool activeState = false;

// Report ID
enum
{
	RID_KEYBOARD = 1,
	RID_MOUSE,
	RID_CONSUMER_CONTROL, // Media, volume etc ..
};

// HID report descriptor using TinyUSB's template
static uint8_t const desc_hid_report[] = {
	TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(RID_KEYBOARD)),
	TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(RID_MOUSE)),
	TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(RID_CONSUMER_CONTROL))};

// USB HID object.
static Adafruit_USBD_HID usb_hid;

static void send_hid_test()
{
	// Whether button is pressed
	bool btn_pressed = (digitalRead(pin) == activeState);

	// Remote wakeup
	if (TinyUSBDevice.suspended() && btn_pressed)
	{
		// Wake up host if we are in suspend mode
		// and REMOTE_WAKEUP feature is enabled by host
		TinyUSBDevice.remoteWakeup();
	}
}

void hid_init()
{
	// Manual begin() is required on core without built-in support e.g. mbed rp2040
	if (!TinyUSBDevice.isInitialized())
	{
		TinyUSBDevice.begin(0);
	}

	// Set up HID
	usb_hid.setPollInterval(2);
	usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
	usb_hid.setStringDescriptor("TinyUSB HID Composite");
	usb_hid.begin();

	// If already enumerated, additional class driver begin() e.g msc, hid, midi won't take effect until re-enumeration
	if (TinyUSBDevice.mounted())
	{
		TinyUSBDevice.detach();
		delay(10);
		TinyUSBDevice.attach();
	}

	// Set up button, pullup opposite to active state
	pinMode(pin, activeState ? INPUT_PULLDOWN : INPUT_PULLUP);

	// SUUUPER DIRTY HACK LOLLLL
	// when this thing starts up the computer seems to think ctrl is held down until you send a ctrl on and ctrl off message
	// here this fires a fake ctrl on and off sequence to stop that shit
	// fire an all-clear on keyboard when usb hid is ready to start things off
	while (!usb_hid.ready())
	{
		delay(1);
		Serial.println("usb_hid not ready yet... waiting to send false ctrl up report!");
	}
	usb_hid.keyboardReport(RID_KEYBOARD, 1, 0);
	while (!usb_hid.ready())
	{
		delay(1);
		Serial.println("usb_hid not ready yet... waiting to send follow-up blank report!");
	}
	usb_hid.keyboardReport(RID_KEYBOARD, 0, 0);
}

static uint8_t held_keys[6] = {0};
static uint8_t held_modifiers = 0;
static bool state_dirty = false;

static void send_keyboard_state()
{

	if (usb_hid.ready())
	{
		state_dirty = false;
#ifdef DEV_BUILD
		Serial.print("sending event as keyboard. held_keys: { ");
		for (size_t i = 0; i < 6; i++)
		{
			Serial.print(held_keys[i], HEX);
			Serial.print(" ");
		}
		Serial.print("}; held_modifiers: ");
		Serial.print(held_modifiers, HEX);
		Serial.print("\n");
#endif
		usb_hid.keyboardReport(RID_KEYBOARD, held_modifiers, held_keys);
	}
	else
	{
#ifdef DEV_BUILD
		Serial.println("usb_hid not ready!!! deferring...");
#endif
		state_dirty = true;
	}
}

void hid_task()
{
#ifdef TINYUSB_NEED_POLLING_TASK
	// Manual call tud_task since it isn't called by Core's background
	TinyUSBDevice.task();
#endif

	// not enumerated()/mounted() yet: nothing to do
	if (!TinyUSBDevice.mounted())
	{
		return;
	}

	// poll gpio once each 10 ms
	static uint32_t ms = 0;
	if (millis() - ms > 10)
	{
		ms = millis();
		send_hid_test();
	}

	if (state_dirty == true)
	{
		send_keyboard_state();
	}
}

static void send_cleared_keyboard_state()
{
	usb_hid.keyboardReport(RID_KEYBOARD, 0, 0);
}

void hid_key_down(uint8_t keycode, uint8_t modifiers)
{
	held_modifiers = modifiers;
	for (int i = 0; i < 6; i++)
	{
		if (held_keys[i] == keycode)
		{
			held_keys[i] = 0;
		}
	}
	for (int i = 0; i < 6; i++)
	{
		if (held_keys[i] == 0)
		{
			held_keys[i] = keycode;
			break;
		} // first empty slot
	}
	send_keyboard_state();
}

void hid_key_up(uint8_t keycode, uint8_t modifiers)
{
	held_modifiers = modifiers;
	for (int i = 0; i < 6; i++)
	{
		if (held_keys[i] == keycode)
		{
			held_keys[i] = 0;
			break;
		}
	}
	send_keyboard_state();
}

void process_keystroke(uint8_t frame_buf[5])
{
	if (frame_buf[0] == 1)
		hid_key_down(frame_buf[1], frame_buf[4]);
	else if (frame_buf[0] == 2)
		hid_key_up(frame_buf[1], frame_buf[4]);
}
