#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include <array>

namespace Catalyst2::Sequencer::Phaser
{

struct Data {
	std::array<Clock::Divider::type, Model::NumChans> cdiv;
	bool Validate() const {
		auto ret = false;
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
		bool sequence_reset;
	};
	std::array<State, Model::NumChans> channel;
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	void Step(uint8_t chan, uint8_t actual_seq_length) {
		auto &c = channel[chan];
		c.clockdivider.Update(data.cdiv[chan]);
		if (c.clockdivider.Step()) {
			c.counter += 1;
			if (c.counter >= actual_seq_length) {
				c.counter = 0;
				c.sequence_reset = true;
			}
		}
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
	bool DidReset(uint8_t chan) {
		const auto ret = channel[chan].sequence_reset;
		channel[chan].sequence_reset = false;
		return ret;
	}
};
} // namespace Catalyst2::Sequencer::Phaser
