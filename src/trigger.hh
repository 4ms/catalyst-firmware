#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "controls.hh"
#include <cstdint>

namespace Catalyst2
{
class Trigger {
	static constexpr auto min_gate_length = 2u, max_gate_length = 500u;
	struct State {
		uint32_t length;
		uint32_t time_trigged;
	};
	std::array<State, Model::NumChans> state;

public:
	void Trig(uint8_t chan, float pulse_width) {
		state[chan].length = CalcTime(pulse_width);
		state[chan].time_trigged = Controls::TimeNow();
	}
	bool Read(uint8_t chan) {
		if (Controls::TimeNow() - state[chan].time_trigged >= state[chan].length) {
			return false;
		}
		return true;
	}

private:
	uint32_t CalcTime(float pulse_width) {
		const auto x = pulse_width;
		return (max_gate_length - min_gate_length) * x * x * x + min_gate_length;
	}
};
} // namespace Catalyst2
