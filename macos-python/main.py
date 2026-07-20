"""
KeyBug — macOS BLE HID relay
Captures keyboard and mouse input globally, sends to KeyBug board over BLE.
Requires Accessibility permission for global input capture.
"""

import asyncio
import os
import signal
import struct
import sys
from pynput import keyboard, mouse
from bleak import BleakClient, BleakScanner

# ── BLE ──────────────────────────────────────────────────────────────────────

DEVICE_NAME    = "keybug!"

# Nordic UART Service
NUS_SERVICE    = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_RX_CHAR    = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # we write here
NUS_TX_CHAR    = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # board writes here

# ── Frame types ───────────────────────────────────────────────────────────────

EVT_KEY_DOWN   = 0x01
EVT_KEY_UP     = 0x02
EVT_MOUSE_MOVE = 0x03
EVT_MOUSE_BTN  = 0x04
EVT_SCROLL     = 0x05

# Frame: [event_type: u8][code: u8][value: i16][value2: i16][modifiers: u8] = 7 bytes
def make_frame(event_type, code=0, value=0, value2=0, modifiers=0):
	return struct.pack("<BBhhB", event_type, code, value, value2, modifiers)

# ── Modifier tracking ─────────────────────────────────────────────────────────

MOD_LEFT_CTRL  = 0x01
MOD_LEFT_SHIFT = 0x02
MOD_LEFT_ALT   = 0x04
MOD_LEFT_GUI   = 0x08  # cmd
MOD_RIGHT_CTRL = 0x10
MOD_RIGHT_SHIFT= 0x20
MOD_RIGHT_ALT  = 0x40
MOD_RIGHT_GUI  = 0x80

KEY_TO_MOD = {
	keyboard.Key.ctrl:    MOD_LEFT_CTRL,
	keyboard.Key.ctrl_r:  MOD_RIGHT_CTRL,
	keyboard.Key.shift:   MOD_LEFT_SHIFT,
	keyboard.Key.shift_r: MOD_RIGHT_SHIFT,
	keyboard.Key.alt:     MOD_LEFT_ALT,
	keyboard.Key.alt_r:   MOD_RIGHT_ALT,
	keyboard.Key.cmd:     MOD_LEFT_GUI,
	keyboard.Key.cmd_r:   MOD_RIGHT_GUI,
}

# ── Key maps ──────────────────────────────────────────────────────────────────

SPECIAL_KEY_MAP = {
	keyboard.Key.space:     0x2C,
	keyboard.Key.enter:     0x28,
	keyboard.Key.backspace: 0x2A,
	keyboard.Key.esc:    	0x29,
	keyboard.Key.tab:       0x2B,
	keyboard.Key.delete:    0x4C,
	keyboard.Key.home:      0x4A,
	keyboard.Key.end:       0x4D,
	keyboard.Key.page_up:   0x4B,
	keyboard.Key.page_down: 0x4E,
	keyboard.Key.up:        0x52,
	keyboard.Key.down:      0x51,
	keyboard.Key.left:      0x50,
	keyboard.Key.right:     0x4F,
	keyboard.Key.caps_lock: 0x39,
	keyboard.Key.f1:        0x3A,
	keyboard.Key.f2:        0x3B,
	keyboard.Key.f3:        0x3C,
	keyboard.Key.f4:        0x3D,
	keyboard.Key.f5:        0x3E,
	keyboard.Key.f6:        0x3F,
	keyboard.Key.f7:        0x40,
	keyboard.Key.f8:        0x41,
	keyboard.Key.f9:        0x42,
	keyboard.Key.f10:       0x43,
	keyboard.Key.f11:       0x44,
	keyboard.Key.f12:       0x45,
	#keyboard.Key.print_screen: 0x46,
	#keyboard.Key.scroll_lock:  0x47,
	#keyboard.Key.pause:        0x48,
	#keyboard.Key.insert:       0x49,
	#keyboard.Key.menu:         0x65,
}

# macOS virtual keycode → HID usage code
VK_TO_HID = {
	0x00: 0x04,  # a
	0x01: 0x16,  # s
	0x02: 0x07,  # d
	0x03: 0x09,  # f
	0x04: 0x0B,  # h
	0x05: 0x0A,  # g
	0x06: 0x1D,  # z
	0x07: 0x1B,  # x
	0x08: 0x06,  # c
	0x09: 0x19,  # v
	0x0B: 0x05,  # b
	0x0C: 0x14,  # q
	0x0D: 0x1A,  # w
	0x0E: 0x08,  # e
	0x0F: 0x15,  # r
	0x10: 0x1C,  # y
	0x11: 0x17,  # t
	0x12: 0x1E,  # 1
	0x13: 0x1F,  # 2
	0x14: 0x20,  # 3
	0x15: 0x21,  # 4
	0x16: 0x23,  # 6
	0x17: 0x22,  # 5
	0x18: 0x2E,  # =
	0x19: 0x26,  # 9
	0x1A: 0x24,  # 7
	0x1B: 0x2D,  # -
	0x1C: 0x25,  # 8
	0x1D: 0x27,  # 0
	0x1E: 0x30,  # ]
	0x1F: 0x12,  # o
	0x20: 0x18,  # u
	0x21: 0x2F,  # [
	0x22: 0x0C,  # i
	0x23: 0x13,  # p
	0x25: 0x0F,  # l
	0x26: 0x0D,  # j
	0x27: 0x34,  # '
	0x28: 0x0E,  # k
	0x29: 0x33,  # ;
	0x2A: 0x31,  # backslash
	0x2B: 0x36,  # ,
	0x2C: 0x38,  # /
	0x2D: 0x11,  # n
	0x2E: 0x10,  # m
	0x2F: 0x37,  # .
	0x32: 0x35,  # `
	0x41: 0x63,  # numpad .
	0x43: 0x55,  # numpad *
	0x45: 0x57,  # numpad +
	0x47: 0x53,  # num lock
	0x4B: 0x54,  # numpad /
	0x4C: 0x58,  # numpad enter
	0x4E: 0x56,  # numpad -
	0x51: 0x67,  # numpad =
	0x52: 0x62,  # numpad 0
	0x53: 0x59,  # numpad 1
	0x54: 0x5A,  # numpad 2
	0x55: 0x5B,  # numpad 3
	0x56: 0x5C,  # numpad 4
	0x57: 0x5D,  # numpad 5
	0x58: 0x5E,  # numpad 6
	0x59: 0x5F,  # numpad 7
	0x5B: 0x60,  # numpad 8
	0x5C: 0x61,  # numpad 9
}

MOUSE_BUTTON_MAP = {
	mouse.Button.left:   0x01,
	mouse.Button.right:  0x02,
	mouse.Button.middle: 0x04,
}

# ── State ─────────────────────────────────────────────────────────────────────

modifiers = 0
event_queue: asyncio.Queue = None
loop: asyncio.AbstractEventLoop = None

def key_to_hid(key):
	"""Returns HID usage code or None."""
	if key in KEY_TO_MOD:
		return None  # handled as modifier, not a keycode
	if key in SPECIAL_KEY_MAP:
		return SPECIAL_KEY_MAP[key]
	if isinstance(key, keyboard.KeyCode) and key.vk is not None:
		return VK_TO_HID.get(key.vk)
	return None

def enqueue(frame):
	"""Thread-safe enqueue from pynput callbacks into asyncio loop."""
	if loop and event_queue:
		loop.call_soon_threadsafe(event_queue.put_nowait, frame)

# ── pynput callbacks ──────────────────────────────────────────────────────────

held_keys: set[int] = set()
held_buttons: set[int] = set()

def on_key_press(key):
	global modifiers
	mod = KEY_TO_MOD.get(key)
	hid = key_to_hid(key)
	if mod:
		modifiers |= mod
		enqueue(make_frame(EVT_KEY_DOWN, code=0, modifiers=modifiers))
		return
	if hid:
		if hid == 6 and modifiers & (MOD_LEFT_CTRL | MOD_RIGHT_CTRL):
			enqueue(make_frame(EVT_KEY_UP, code=0, modifiers=0))
			os.kill(os.getpid(), signal.SIGINT)
			return
		enqueue(make_frame(EVT_KEY_DOWN, code=hid, modifiers=modifiers))

def on_key_release(key):
	global modifiers
	mod = KEY_TO_MOD.get(key)
	if mod:
		modifiers &= ~mod
		enqueue(make_frame(EVT_KEY_UP, code=0, modifiers=modifiers))
		return
	hid = key_to_hid(key)
	if hid:
		enqueue(make_frame(EVT_KEY_UP, code=hid, modifiers=modifiers))

def on_mouse_move(x, y):
	# pynput gives absolute coords; we need to track and compute deltas
	pass  # handled by on_mouse_move_delta below

_last_mouse = None

def on_mouse_move_delta(x, y):
	global _last_mouse
	if _last_mouse is None:
		_last_mouse = (x, y)
		return
	dx = int(x - _last_mouse[0])
	dy = int(y - _last_mouse[1])
	_last_mouse = (x, y)
	if dx == 0 and dy == 0:
		return
	enqueue(make_frame(EVT_MOUSE_MOVE, value=dx, value2=dy))

def on_mouse_click(x, y, button, pressed):
	btn = MOUSE_BUTTON_MAP.get(button, 0)
	enqueue(make_frame(EVT_MOUSE_BTN, code=btn, value=1 if pressed else 0))

def on_mouse_scroll(x, y, dx, dy):
	enqueue(make_frame(EVT_SCROLL, value=int(dy)))

# ── BLE ───────────────────────────────────────────────────────────────────────

async def find_device():
	print(f"Scanning for '{DEVICE_NAME}'...")
	device = await BleakScanner.find_device_by_name(DEVICE_NAME, timeout=10.0)
	if device is None:
		print(f"Device '{DEVICE_NAME}' not found. Is it advertising?")
		sys.exit(1)
	print(f"Found: {device.name} [{device.address}]")
	return device

async def send_loop(client: BleakClient):
	"""Drain the event queue and send frames over BLE."""
	print("Input relay active. Ctrl+C to stop.")
	while True:
		frame = await event_queue.get()
		try:
			await client.write_gatt_char(NUS_RX_CHAR, frame, response=False)
		except Exception as e:
			print(f"BLE write error: {e}")
			break

async def main():
	global event_queue, loop
	loop = asyncio.get_running_loop()
	event_queue = asyncio.Queue()

	device = await find_device()

	async with BleakClient(device) as client:
		print(f"Connected to {device.name}")

		# start input listeners
		kb_listener = keyboard.Listener(
			on_press=on_key_press,
			on_release=on_key_release,
			suppress=True,  # consume events — don't also act on local machine
		)
		ms_listener = mouse.Listener(
			on_move=on_mouse_move_delta,
			on_click=on_mouse_click,
			on_scroll=on_mouse_scroll,
		)
		kb_listener.start()
		ms_listener.start()

		try:
			await send_loop(client)
		except asyncio.CancelledError:
			pass
		finally:
			kb_listener.stop()
			#ms_listener.stop()
			print("Disconnected.")

if __name__ == "__main__":
	try:
		asyncio.run(main())
	except KeyboardInterrupt:
		print("\nStopped.")