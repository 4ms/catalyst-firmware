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

class Retrigger {
	uint16_t period;
	uint16_t cnt;
	uint8_t remaining = 0;
	bool out = false;

public:
	void Trig(uint8_t retrig_cnt, uint8_t clock_div) {
		remaining = retrig_cnt;
		period = (Model::clock_mult_factor * clock_div) / (retrig_cnt + 1);
		cnt = 0;
		out = true;
	}
	void Update() {
		cnt++;
		if (remaining && cnt >= period) {
			cnt = 0;
			remaining -= 1;
			out = true;
		}
	}
	bool Read() {
		const auto o = out;
		out = false;
		return o;
	}
};

} // namespace Catalyst2