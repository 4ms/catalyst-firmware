#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "controls.hh"
#include <cstdint>

namespace Catalyst2
{
class Trigger {
	static constexpr auto trig_length = Clock::MsToTicks(Model::triglengthms);
	uint8_t trig_time;
	bool is_trigged = false;

public:
	void Trig(uint8_t time_now) {
		trig_time = time_now;
		is_trigged = true;
	}

	bool Read(uint8_t chan, uint32_t time_now) const {
		auto &s = state[chan];
		if (time_now - s.trig_time >= s.length) {
			return false;
		}
		return true;
	}
};

class Retrigger {
	uint16_t period;
	uint16_t cnt;
	uint8_t remaining = 0;
	bool out = false;

private:
	uint32_t CalculateLength(float pw) const {
		return Clock::MsToTicks(pw * pw * pw * 500 + 1);
	}
};

