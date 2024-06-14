#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "controls.hh"
#include <cstdint>

namespace Catalyst2
{
class Trigger {
	static constexpr auto min_gate_length = 2u;
	static constexpr auto max_gate_length = 500u;
	static constexpr auto sustain = 0xffffffffu;

	static constexpr auto sustain_max_length =
		Clock::TicksToMs(0xffffffff) / 1000.f / 60.f / 60.f / 24.f / 7.f; // weeks

	struct State {
		uint32_t time_trigged;
	};
	std::array<State, Model::NumChans> state;

public:
	void Trig(uint8_t chan) {
		state[chan].time_trigged = Controls::TimeNow();
	}
	bool Read(uint8_t chan, float pulse_width) {
		if (Controls::TimeNow() - state[chan].time_trigged >= CalcTime(pulse_width)) {
			return false;
		}
		return true;
	}

private:
	uint32_t CalcTime(float pulse_width) {
		if (pulse_width >= 1.f) {
			return sustain;
		}
		if (pulse_width <= 0.f) {
			return 0;
		}
		const auto x = pulse_width;
		return Clock::MsToTicks((max_gate_length - min_gate_length) * x * x * x + min_gate_length);
	}
};
} // namespace Catalyst2
