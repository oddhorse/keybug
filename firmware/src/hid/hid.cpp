#include "hid.h"

#include <Arduino.h>
#include "Adafruit_TinyUSB.h"
#include "frame_queue.h"
#include <frame.h>

/* Demo button -> HID reports, moved as-is from the stock Adafruit example.
 * Depending on the board, the button pin and its active state (when
 * pressed) are different.
 */

static const int pin = PIN_BUTTON1;
static bool activeState = false;

// HID report descriptors using TinyUSB's template
uint8_t const desc_keyboard_report[] = {
	TUD_HID_REPORT_DESC_KEYBOARD()};
uint8_t const desc_mouse_report[] = {
	TUD_HID_REPORT_DESC_MOUSE()};

// USB HID objects
Adafruit_USBD_HID usb_keyboard;
Adafruit_USBD_HID usb_mouse;

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

	// HID Keyboard
	usb_keyboard.setPollInterval(2);
	usb_keyboard.setBootProtocol(HID_ITF_PROTOCOL_KEYBOARD);
	usb_keyboard.setReportDescriptor(desc_keyboard_report, sizeof(desc_keyboard_report));
	usb_keyboard.setStringDescriptor("keybug");
	usb_keyboard.begin();

	// HID Mouse
	usb_mouse.setPollInterval(2);
	usb_mouse.setBootProtocol(HID_ITF_PROTOCOL_MOUSE);
	usb_mouse.setReportDescriptor(desc_mouse_report, sizeof(desc_mouse_report));
	usb_mouse.setStringDescriptor("mousebug");
	usb_mouse.begin();

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
	//
	// Bounded wait: some hosts (BIOS/legacy USB) never report ready() at all —
	// this must not hang setup()/loop() forever if that happens, or NOTHING on
	// the board works, not just this workaround.
	const uint32_t READY_TIMEOUT_MS = 2000;
	uint32_t wait_start = millis();
	while (!usb_keyboard.ready() && millis() - wait_start < READY_TIMEOUT_MS)
	{
		delay(1);
	}
	if (usb_keyboard.ready())
	{
		usb_keyboard.keyboardReport(0, 1, 0);
		wait_start = millis();
		while (!usb_keyboard.ready() && millis() - wait_start < READY_TIMEOUT_MS)
		{
			delay(1);
		}
		if (usb_keyboard.ready())
			usb_keyboard.keyboardReport(0, 0, 0);
	}
	else
	{
		Serial.println("usb_keyboard never became ready — skipping stuck-ctrl workaround");
	}
}

bool hid_keyboard_ready()
{
	return usb_keyboard.ready();
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
	usb_keyboard.keyboardReport(0, held_modifiers, held_keys);
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

static int8_t clamp_i8(int16_t v)
{
	if (v > 127)
		return 127;
	if (v < -128)
		return -128;
	return (int8_t)v;
}

// hid devices send complete current button state as one bitmask, so we need to track state
static uint8_t held_mouse_buttons = 0;

// Single-shot: clamps to what one HID report can carry. Doesn't split a large
// delta across multiple reports/ticks — see AGENTS.md frame format notes.
// TODO: implement splitting
void hid_mouse_move(int16_t dx, int16_t dy)
{
	// switched from usb_mouse.mouseMove() because that sends a whole report with held buttons as 0
	usb_mouse.mouseReport(0, held_mouse_buttons, clamp_i8(dx), clamp_i8(dy), 0, 0);
}

void hid_mouse_press(uint8_t btn)
{
	held_mouse_buttons |= btn;
	usb_mouse.mouseButtonPress(0, held_mouse_buttons);
}

void hid_mouse_release(uint8_t btn)
{
	held_mouse_buttons &= ~btn;
	usb_mouse.mouseButtonPress(0, held_mouse_buttons);
}

void hid_mouse_handle(uint8_t btn, int16_t val)
{
	if (val == 1)
		hid_mouse_press(btn);
	else if (val == 0)
		hid_mouse_release(btn);
}

void hid_mouse_scroll(int16_t vert, int16_t horiz)
{
	/*
	i think this zeroes out any held buttons and movements, but it seems like
	the macos trackpad literally can't send scrolls at the same time as buttons OR movements
	*/
	usb_mouse.mouseScroll(0, vert, horiz);
}

void hid_keyboard_clear()
{
	memset(held_keys, 0, sizeof(held_keys));
	held_modifiers = 0;
	send_keyboard_state();
}

void hid_mouse_clear()
{
	held_mouse_buttons = 0;
	usb_mouse.mouseReport(0, held_mouse_buttons, 0, 0, 0, 0);
}

/**
 * we're making a queue so we can process all the frames in time
 */
static FrameQueue<16> kbd_queue;
static FrameQueue<16> mouse_queue;

/**
 * receives input frame from ble stream and queues it
 */
void enqueue_frame(uint8_t raw_frame[7])
{
	Frame frame = parse_frame(raw_frame);
	bool kbd_push_success,
		mouse_push_success;

	switch (frame.event_type)
	{
	case EventType::KeyDown:
	case EventType::KeyUp:
	case EventType::ConsumerDown:
	case EventType::ConsumerUp:
		kbd_push_success = kbd_queue.push(frame);
		if (!kbd_push_success)
		{
			// TODO handle push failure
		}
		break;
	case EventType::MouseMove:
	{
		// vars for amounts left to move
		int16_t xIter = frame.value;
		int16_t yIter = frame.value2;

		// while there's still movement left to make
		while (xIter != 0 || yIter != 0)
		{
			Frame step_frame = frame;
			// vars for amounts to move this time around
			int8_t xStep, yStep;

			if (xIter > 127)
			{
				xStep = 127;
				xIter -= 127;
			}
			else if (xIter < -128)
			{
				xStep = -128;
				xIter += 128;
			}
			else
			{
				xStep = xIter;
				xIter = 0;
			}
			if (yIter > 127)
			{
				yStep = 127;
				yIter -= 127;
			}
			else if (yIter < -128)
			{
				yStep = -128;
				yIter += 128;
			}
			else
			{
				yStep = yIter;
				yIter = 0;
			}

			step_frame.value = xStep;
			step_frame.value2 = yStep;

			mouse_push_success = mouse_queue.push(step_frame);
			if (!mouse_push_success)
			{
				// TODO handle push failure
			}
		}
		break;
	}
	case EventType::MouseButton:
	case EventType::Scroll:
		mouse_push_success = mouse_queue.push(frame);
		if (!mouse_push_success)
		{
			// TODO handle push failure
		}
		break;
	case EventType::Clear:
		kbd_push_success = kbd_queue.push(frame);
		mouse_push_success = mouse_queue.push(frame);
		if (!kbd_push_success || !mouse_push_success)
		{
			// TODO handle push failure
		}
		break;
	default:
		break;
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

	// process frames whenever hids are ready

	// if keyboard data is in queue
	if (kbd_queue.get_count() != 0)
	{
		if (usb_keyboard.ready())
		{
			Frame frame;
			// if queue pop was successful, process the frame pulled
			if (kbd_queue.pop(frame))
			{
				switch (frame.event_type)
				{
				case EventType::KeyDown:
					hid_key_down(frame.code, frame.modifiers);
					break;
				case EventType::KeyUp:
					hid_key_up(frame.code, frame.modifiers);
					break;
				case EventType::ConsumerDown:
				case EventType::ConsumerUp:
				case EventType::Clear:
					hid_keyboard_clear();
					break;
				default:
					break;
				}
			}
		}
		else
		{
#ifdef DEV_BUILD
			Serial.println("usb_keyboard not ready! waiting...");
#endif
		}
	}

	// if mouse data is in queue
	if (mouse_queue.get_count() != 0)
	{
		if (usb_mouse.ready())
		{
			Frame frame;
			// if queue pop was successful, process the frame pulled
			if (mouse_queue.pop(frame))
				switch (frame.event_type)
				{
				case EventType::MouseMove:
					hid_mouse_move(frame.value, frame.value2);
					break;
				case EventType::MouseButton:
					hid_mouse_handle(frame.code, frame.value);
					break;
				case EventType::Scroll:
					hid_mouse_scroll(frame.value, frame.value2);
					break;
				case EventType::Clear:
					hid_mouse_clear();
					break;
				default:
					break;
				}
		}
		else
		{
#ifdef DEV_BUILD
			Serial.println("usb_mouse not ready! waiting...");
#endif
		}
	}
}