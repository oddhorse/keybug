/**
 * frame_queue.h
 * queue to store incoming frames before hid translation/sending
 */

#pragma once
#include <Arduino.h>

template <size_t Capacity>
class FrameQueue
{
public:
	bool push(const uint8_t frame[7])
	{
		if (count_ == Capacity)
			return false;
		memcpy(queue_[tail_], frame, 7);
		tail_ = (tail_ + 1) % Capacity;
		count_++;
		return true;
	}

	bool pop(uint8_t frame[7])
	{
		if (count_ == 0)
			return false;
		memcpy(frame, queue_[head_], 7);
		head_ = (head_ + 1) % Capacity;
		count_--;
		return true;
	}

	uint8_t get_count()
	{
		return count_;
	}

private:
	uint8_t queue_[Capacity][7] = {};
	uint8_t head_ = 0, tail_ = 0, count_ = 0;
};