#pragma once

#include "controls.hh"
#include <cstdint>

namespace Catalyst2
{
class Trigger {
	static constexpr uint8_t trig_length = 20;
	uint8_t trig_time;
	bool is_trigged = false;

public:
	void Trig(uint8_t time_now) {
		trig_time = time_now;
		is_trigged = true;
	}
	bool Read(uint8_t time_now) {
		if (!is_trigged)
			return false;
		const uint8_t elapsed = time_now - trig_time;
		if (elapsed >= trig_length) {
			is_trigged = false;
			return false;
		}
		return true;
	}
};
} // namespace Catalyst2