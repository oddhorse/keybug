//
//  HIDKeyMap.swift
//  keybug
//
//  Created by john on 7/20/26.
//

import Carbon.HIToolbox

let VK_TO_HID: [Int: UInt8] = [
    kVK_ANSI_A:                 0x04,  // a
    kVK_ANSI_S:                 0x16,  // s
    kVK_ANSI_D:                 0x07,  // d
    kVK_ANSI_F:                 0x09,  // f
    kVK_ANSI_H:                 0x0B,  // h
    kVK_ANSI_G:                 0x0A,  // g
    kVK_ANSI_Z:                 0x1D,  // z
    kVK_ANSI_X:                 0x1B,  // x
    kVK_ANSI_C:                 0x06,  // c
    kVK_ANSI_V:                 0x19,  // v
    kVK_ANSI_B:                 0x05,  // b
    kVK_ANSI_Q:                 0x14,  // q
    kVK_ANSI_W:                 0x1A,  // w
    kVK_ANSI_E:                 0x08,  // e
    kVK_ANSI_R:                 0x15,  // r
    kVK_ANSI_Y:                 0x1C,  // y
    kVK_ANSI_T:                 0x17,  // t
    kVK_ANSI_1:                 0x1E,  // 1
    kVK_ANSI_2:                 0x1F,  // 2
    kVK_ANSI_3:                 0x20,  // 3
    kVK_ANSI_4:                 0x21,  // 4
    kVK_ANSI_6:                 0x23,  // 6
    kVK_ANSI_5:                 0x22,  // 5
    kVK_ANSI_Equal:             0x2E,  // =
    kVK_ANSI_9:                 0x26,  // 9
    kVK_ANSI_7:                 0x24,  // 7
    kVK_ANSI_Minus:             0x2D,  // -
    kVK_ANSI_8:                 0x25,  // 8
    kVK_ANSI_0:                 0x27,  // 0
    kVK_ANSI_RightBracket:      0x30,  // ]
    kVK_ANSI_O:                 0x12,  // o
    kVK_ANSI_U:                 0x18,  // u
    kVK_ANSI_LeftBracket:       0x2F,  // [
    kVK_ANSI_I:                 0x0C,  // i
    kVK_ANSI_P:                 0x13,  // p
    kVK_ANSI_L:                 0x0F,  // l
    kVK_ANSI_J:                 0x0D,  // j
    kVK_ANSI_Quote:             0x34,  // '
    kVK_ANSI_K:                 0x0E,  // k
    kVK_ANSI_Semicolon:         0x33,  // ;
    kVK_ANSI_Backslash:         0x31,  // backslash
    kVK_ANSI_Comma:             0x36,  // ,
    kVK_ANSI_Slash:             0x38,  // /
    kVK_ANSI_N:                 0x11,  // n
    kVK_ANSI_M:                 0x10,  // m
    kVK_ANSI_Period:            0x37,  // .
    kVK_ANSI_Grave:             0x35,  // `
    kVK_ANSI_KeypadDecimal:     0x63,  // numpad .
    kVK_ANSI_KeypadMultiply:    0x55,  // numpad *
    kVK_ANSI_KeypadPlus:        0x57,  // numpad +
    kVK_ANSI_KeypadClear:       0x53,  // num lock (Clear key on Mac keypads)
    kVK_ANSI_KeypadDivide:      0x54,  // numpad /
    kVK_ANSI_KeypadEnter:       0x58,  // numpad enter
    kVK_ANSI_KeypadMinus:       0x56,  // numpad -
    kVK_ANSI_KeypadEquals:      0x67,  // numpad =
    kVK_ANSI_Keypad0:           0x62,  // numpad 0
    kVK_ANSI_Keypad1:           0x59,  // numpad 1
    kVK_ANSI_Keypad2:           0x5A,  // numpad 2
    kVK_ANSI_Keypad3:           0x5B,  // numpad 3
    kVK_ANSI_Keypad4:           0x5C,  // numpad 4
    kVK_ANSI_Keypad5:           0x5D,  // numpad 5
    kVK_ANSI_Keypad6:           0x5E,  // numpad 6
    kVK_ANSI_Keypad7:           0x5F,  // numpad 7
    kVK_ANSI_Keypad8:           0x60,  // numpad 8
    kVK_ANSI_Keypad9:           0x61,  // numpad 9

    // Specials (Python SPECIAL_KEY_MAP, merged — CGEventTap keys everything by vk)
    kVK_Space:                  0x2C,  // space
    kVK_Return:                 0x28,  // enter
    kVK_Delete:                 0x2A,  // backspace (Carbon "Delete" = HID Backspace!)
    kVK_Escape:                 0x29,  // esc
    kVK_Tab:                    0x2B,  // tab
    kVK_ForwardDelete:          0x4C,  // delete (fn+delete / ⌦)
    kVK_Home:                   0x4A,  // home
    kVK_End:                    0x4D,  // end
    kVK_PageUp:                 0x4B,  // page up
    kVK_PageDown:               0x4E,  // page down
    kVK_UpArrow:                0x52,  // up
    kVK_DownArrow:              0x51,  // down
    kVK_LeftArrow:              0x50,  // left
    kVK_RightArrow:             0x4F,  // right
    kVK_CapsLock:               0x39,  // caps lock — arrives via .flagsChanged, not .keyDown
    kVK_F1:                     0x3A,  // f1
    kVK_F2:                     0x3B,  // f2
    kVK_F3:                     0x3C,  // f3
    kVK_F4:                     0x3D,  // f4
    kVK_F5:                     0x3E,  // f5
    kVK_F6:                     0x3F,  // f6
    kVK_F7:                     0x40,  // f7
    kVK_F8:                     0x41,  // f8
    kVK_F9:                     0x42,  // f9
    kVK_F10:                    0x43,  // f10
    kVK_F11:                    0x44,  // f11
    kVK_F12:                    0x45,  // f12
]

// ── Modifier bits (HID modifier byte layout, mirrors Python MOD_*) ────────────

let MOD_LEFT_CTRL:   UInt8 = 0x01
let MOD_LEFT_SHIFT:  UInt8 = 0x02
let MOD_LEFT_ALT:    UInt8 = 0x04
let MOD_LEFT_GUI:    UInt8 = 0x08  // cmd
let MOD_RIGHT_CTRL:  UInt8 = 0x10
let MOD_RIGHT_SHIFT: UInt8 = 0x20
let MOD_RIGHT_ALT:   UInt8 = 0x40
let MOD_RIGHT_GUI:   UInt8 = 0x80

// Modifier keys only generate .flagsChanged events (never .keyDown/.keyUp);
// the event's keycode tells us which physical key, hence left/right.
// Mirrors Python KEY_TO_MOD.
let VK_TO_MOD: [Int: UInt8] = [
    kVK_Control:      MOD_LEFT_CTRL,
    kVK_RightControl: MOD_RIGHT_CTRL,
    kVK_Shift:        MOD_LEFT_SHIFT,
    kVK_RightShift:   MOD_RIGHT_SHIFT,
    kVK_Option:       MOD_LEFT_ALT,
    kVK_RightOption:  MOD_RIGHT_ALT,
    kVK_Command:      MOD_LEFT_GUI,
    kVK_RightCommand: MOD_RIGHT_GUI,
]

// ── Mouse buttons (CGEvent .mouseEventButtonNumber → HID button bit) ──────────

let MOUSE_BUTTON_MAP: [Int: UInt8] = [
    0: 0x01,  // left
    1: 0x02,  // right
    2: 0x04,  // middle
]
