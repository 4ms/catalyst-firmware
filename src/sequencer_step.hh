#pragma once

#include "channel.hh"
#include <bit>
#include <cstdint>

namespace Catalyst2::Sequencer
{
struct TrigDelay {
	using type = int8_t;
	static constexpr type min = -31, max = 31;
	static constexpr type bits = std::bit_width(static_cast<uint32_t>(max - min));
};
struct MorphRetrig {
	using type = uint8_t;
	static constexpr type min = 0u, max = 15u, bits = std::bit_width(max);
};
struct Probability {
	using type = uint8_t;
	static constexpr type min = 0u, max = 15u, bits = std::bit_width(max);
	static constexpr float toFloat(type p) {
		return p / static_cast<float>(max);
	}
};
struct Step {
	uint32_t cv : Channel::Cv::bits = Channel::Cv::zero;
	int32_t trig_delay : TrigDelay::bits = 0;
	uint32_t gate : Channel::Gate::bits = 0;
	uint32_t morph_retrig : MorphRetrig::bits = 0;
	uint32_t probability : Probability::bits = 0;

public:
	Channel::Cv::type ReadCv(float random = 0.f) const {
		return Channel::Cv::Read(cv, random);
	}
	void IncCv(int32_t inc, bool fine, Channel::Cv::Range range) {
		cv = Channel::Cv::Inc(cv, inc, fine, range);
	}
	float ReadGate(float random = 0.f) const {
		return Channel::Gate::Read(gate, random);
	}
	void IncGate(int32_t inc) {
		gate = Channel::Gate::Inc(gate, inc);
	}
	float ReadTrigDelay() const {
		return trig_delay / static_cast<float>(TrigDelay::max + 1);
	}
	void IncTrigDelay(int32_t inc) {
		trig_delay = std::clamp<int32_t>(trig_delay + inc, TrigDelay::min, TrigDelay::max);
	}
	float ReadMorph() const {
		return morph_retrig / static_cast<float>(MorphRetrig::max);
	}
	MorphRetrig::type ReadRetrig() const {
		return morph_retrig;
	}
	void IncMorphRetrig(int32_t inc) {
		morph_retrig = std::clamp<int32_t>(morph_retrig + inc, MorphRetrig::min, MorphRetrig::max);
	}
	Probability::type ReadProbability() const {
		return probability;
	}
	void IncProbability(int32_t inc) {
		probability = std::clamp<int32_t>(probability + inc, Probability::min, Probability::max);
	}
	bool Validate() const {
		auto ret = true;
		ret &= Channel::Cv::Validate(cv);
		ret &= Channel::Gate::Validate(gate);
		// the rest will always be true
		return ret;
	}
};

static_assert(sizeof(Step) <= 4);

} // namespace Catalyst2::Sequencer
