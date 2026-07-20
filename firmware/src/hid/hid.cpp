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

static void send_keyboard_state()
{
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

void hid_mouse_move(uint16_t dx, uint16_t dy)
{
	usb_hid.mouseMove(RID_MOUSE, dx, dy);
}

/**
 * we're making a queue so we can process all the frames in time
 */
#define QUEUE_SIZE 16
static uint8_t queue[QUEUE_SIZE][7];
static uint8_t queue_head = 0;
static uint8_t queue_tail = 0;
static uint8_t queue_count = 0;

static bool queue_push(const uint8_t frame[7])
{
	if (queue_count == QUEUE_SIZE)
		return false;
	memcpy(queue[queue_tail], frame, 7);
	queue_tail = (queue_tail + 1) % QUEUE_SIZE;
	queue_count++;
	return true;
}

static bool queue_pop(uint8_t frame[7])
{
	if (queue_count == 0)
		return false;
	memcpy(frame, queue[queue_head], 7);
	queue_head = (queue_head + 1) % QUEUE_SIZE;
	queue_count--;
	return true;
}

/**
 * receives input frame from ble stream and queues it
 */
void enqueue_frame(uint8_t frame[7])
{
	bool push_success = queue_push(frame);
	if (!push_success)
	{
		// [TODO] handle push failure
	}
}

void process_frame(uint8_t frame[7])
{
	// Frame: [0 event_type: u8][1 code: u8][2/3 value: i16][4/5 value2: i16][6 modifiers: u8] = 7 bytes
	// key down
	if (frame[0] == 1)
		hid_key_down(frame[1], frame[6]);
	// key up
	else if (frame[0] == 2)
		hid_key_up(frame[1], frame[6]);
	// mouse move
	else if (frame[0] == 3)
	{
		int16_t value = (int16_t)((uint16_t)frame[2] | ((uint16_t)frame[3] << 8));
		int16_t value2 = (int16_t)((uint16_t)frame[4] | ((uint16_t)frame[5] << 8));
		hid_mouse_move(value, value2);
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

	// process frames whenever usb_hid is ready
	if (usb_hid.ready())
	{
		uint8_t pulled_frame[7];
		// if queue pop was successful, process the frame pulled
		if (queue_pop(pulled_frame))
			process_frame(pulled_frame);
	}
	else
	{
#ifdef DEV_BUILD
		Serial.println("usb_hid not ready! waiting...");
#endif
	}
}