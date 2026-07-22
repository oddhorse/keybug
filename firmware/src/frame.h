/**
 * frame.h
 * defines pretty types for transmission frames and event types in them
 * by oddhorse / john trinh
 */

#pragma once

#include <Arduino.h>

enum class EventType : uint8_t
{
	KeyDown = 1,
	KeyUp = 2,
	MouseMove = 3,
	MouseButton = 4,
	Scroll = 5,
	ConsumerDown = 6,
	ConsumerUp = 7,
	Clear = 8,
	Control = 9
};

struct Frame
{
	EventType event_type;
	uint8_t code;
	int16_t value;
	int16_t value2;
	uint8_t modifiers;
};

Frame parse_frame(const uint8_t raw[7])
{
	Frame out;
	out.event_type = static_cast<EventType>(raw[0]);
	out.code = raw[1];
	out.value = (int16_t)((uint16_t)raw[2] | ((uint16_t)raw[3] << 8));
	out.value2 = (int16_t)((uint16_t)raw[4] | ((uint16_t)raw[5] << 8));
	out.modifiers = raw[6];
	return out;
}