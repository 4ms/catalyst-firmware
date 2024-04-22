#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include <array>

namespace Catalyst2::Sequencer::Phaser
{

struct Data {
	std::array<Clock::Divider::type, Model::NumChans> cdiv;
	bool Validate() const {
		auto ret = true;
		for (auto &c : cdiv) {
			ret &= c.Validate();
		}
		return ret;
	}
};

class Interface {
	struct State {
		Clock::Divider clockdivider;
		uint8_t counter = 0;
	};
	std::array<State, Model::NumChans> channel;
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	bool Step(uint8_t chan, uint8_t actual_seq_length) {
		auto &c = channel[chan];
		if (c.clockdivider.Update(data.cdiv[chan])) {
			c.counter += 1;
			if (c.counter >= actual_seq_length) {
				c.counter = 0;
				return true;
			}
		}
		return false;
	}
	float GetPhase(uint8_t chan, float clock_phase) const {
		return channel[chan].counter + channel[chan].clockdivider.GetPhase(data.cdiv[chan], clock_phase);
	}
	void Reset() {
		for (auto &c : channel) {
			c.counter = 0;
			c.clockdivider.Reset();
		}
	}
};
} // namespace Catalyst2::Sequencer::Phaser
