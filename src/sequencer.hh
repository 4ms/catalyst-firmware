#pragma once

#include "channel.hh"
#include "conf/model.hh"
#include "random.hh"
#include "sequencer_player.hh"
#include "sequencer_settings.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Sequencer
{

class StepModifier {
	static constexpr uint8_t min = 0, max = 7;
	uint8_t m = 0;

public:
	float AsMorph() {
		return m / static_cast<float>(max);
	}
	uint8_t AsRetrig() {
		return m >> 1;
	}
	void Inc(int32_t inc, bool is_gate) {
		if (is_gate) {
			inc *= 2;
		}
		m = std::clamp<int32_t>(m + inc, min, max - 1);
	}
	bool Validate() {
		return m <= max;
	}
};

struct Step : Channel::Value {
	StepModifier modifier;
};

struct ChannelData : public std::array<Step, Model::MaxSeqSteps> {
	bool Validate() {
		for (auto &s : *this) {
			if (!s.modifier.Validate()) {
				return false;
			}
		}
		return true;
	}
};

struct Data {
	std::array<Sequencer::ChannelData, Model::NumChans> channel;
	Sequencer::Settings::Data settings;
	Random::Pool::SeqData randompool{};

	bool validate() {
		for (auto &c : channel) {
			if (!c.Validate()) {
				return false;
			}
		}
		return settings.Validate();
	}
};

} // namespace Catalyst2::Sequencer
