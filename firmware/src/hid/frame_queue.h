/**
 * frame_queue.h
 * queue to store incoming frames before hid translation/sending
 */

#pragma once
#include <Arduino.h>
#include "frame.h"

template <size_t Capacity>
class FrameQueue
{
public:
	bool push(const Frame &frame)
	{
		if (count_ == Capacity)
		{
#ifdef DEV_BUILD
			Serial.println("queue is out of space!! dropping frame");
			Serial.println("frame dropped:");
			print_frame(frame);
#endif
			return false;
		}
		queue_[tail_] = frame;
		tail_ = (tail_ + 1) % Capacity;
		count_++;
		return true;
	}

	bool pop(Frame &frame)
	{
		if (count_ == 0)
			return false;
		frame = queue_[head_];
		head_ = (head_ + 1) % Capacity;
		count_--;
		return true;
	}

	uint8_t get_count()
	{
		return count_;
	}

private:
	Frame queue_[Capacity] = {};
	uint8_t head_ = 0, tail_ = 0, count_ = 0;
};