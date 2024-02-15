#pragma once

#include "conf/model.hh"
#include "sequencer_step.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <span>

namespace Catalyst2::Random
{
namespace Amount
{
using type = float;
inline constexpr auto min = 0.f;
inline constexpr auto max = 1.f;

// +/1 2 semitones
inline constexpr auto inc = (max / (Model::output_octave_range * 12)) * 2;
inline constexpr auto def = inc;
} // namespace Amount
template<typename T>
inline void RandomizeBuffer(T &d) {
	for (auto &r : d) {
		r = std::rand();
	}
}

namespace Macro::Pool
{

using type = float;

struct Data : public std::array<int8_t, Model::Macro::NumScenes * Model::NumChans> {
	Data() {
		RandomizeBuffer(*this);
	}
	bool Validate() const {
		return true;
	}
};

class Interface {
	using T = Data;
	T &data;
	bool is_random = true;

public:
	Interface(T &data)
		: data{data} {
	}
	uint8_t GetSeed() const {
		return is_random ? data[0] + 128 : 0;
	}
	void Clear() {
		is_random = false;
	}
	void Randomize() {
		is_random = true;
		RandomizeBuffer(data);
	}
	bool IsRandomized() const {
		return is_random;
	}
	type Read(uint8_t channel, uint8_t step_or_scene, float amnt) const {
		if (!is_random) {
			return 0.f;
		}
		float t;
		t = data[(channel * Model::NumChans) + step_or_scene];
		t /= 128.f;
		return t * amnt;
	}
};
} // namespace Macro::Pool
namespace Sequencer
{
namespace Probability
{
using type = Catalyst2::Sequencer::Probability::type;

class Interface {
	struct ProbStep {
		int8_t val;
		type diceroll : Catalyst2::Sequencer::Probability::bits;
	};
	struct Buffer {
		ProbStep pp;
		ProbStep prev;
		ProbStep cur;
		ProbStep next;
	};
	std::array<Buffer, Model::NumChans> buffer;
	// TODO: std::array<std::array<ProbStep, 4>, Model::NumChans> buffer;

public:
	float ReadRelative(uint8_t chan, int8_t relative_pos, type step_prob_threshold) const {
		// TODO: auto step = buffer[chan][std::clamp(relative_pos+1, -1, 1)];
		if (step_prob_threshold == 0)
			return 0;

		auto step = relative_pos == -1 ? buffer[chan].prev : relative_pos == 1 ? buffer[chan].next : buffer[chan].cur;
		if (step.diceroll >= step_prob_threshold)
			return 0;
		else
			return step.val ? step.val / static_cast<float>(INT8_MIN) : -1.f;
	}
	void Step(uint8_t chan) {
		buffer[chan].pp = buffer[chan].prev;
		buffer[chan].prev = buffer[chan].cur;
		buffer[chan].cur = buffer[chan].next;
		const auto r = std::rand();
		buffer[chan].next.diceroll = r & 0x0f;
		buffer[chan].next.val = r / 256;
	}
	void StepBackward(uint8_t chan) {
		buffer[chan].next = buffer[chan].cur;
		buffer[chan].cur = buffer[chan].prev;
		buffer[chan].prev = buffer[chan].pp;
		const auto r = std::rand();
		buffer[chan].pp.diceroll = r & 0x0f;
		buffer[chan].pp.val = r / 256;
	}
};
} // namespace Probability
namespace Steps
{
struct Data : public std::array<std::array<uint8_t, Model::Sequencer::Steps::Max>, Model::NumChans> {
	Data() {
		for (auto &c : *this) {
			RandomizeBuffer(c);
		}
	}
	bool Validate() const {
		return true;
	}
};
class Interface {
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	void Randomize(uint8_t chan) {
		RandomizeBuffer(data[chan]);
	}
	void Randomize() {
		data = Data{};
	}
	uint8_t Read(uint8_t channel, uint8_t step) const {
		return data[channel][step];
	}
};
} // namespace Steps
} // namespace Sequencer
} // namespace Catalyst2::Random
