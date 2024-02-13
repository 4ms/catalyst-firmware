#pragma once

#include "channel.hh"
#include <bit>
#include <cstdint>

namespace Catalyst2::Sequencer
{
struct TrigDelay {
	using type = int8_t;
	static constexpr type min = -31;
	static constexpr type max = 31;
	static constexpr type bits = std::bit_width(static_cast<uint32_t>(max - min));
};
struct MorphRetrig {
	using type = uint8_t;
	static constexpr type min = 0u;
	static constexpr type max = 15u;
	static constexpr type bits = std::bit_width(max);
};
struct Probability {
	using type = uint8_t;
	static constexpr type min = 0u;
	static constexpr type max = 15u;
	static constexpr type bits = std::bit_width(max);
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
		static constexpr std::array gate_widths = {0.f,
												   0.004f,
												   0.032f,
												   0.064f,
												   0.128f,
												   0.256f,
												   7.f / 16.f,
												   8.f / 16.f,
												   9.f / 16.f,
												   10.f / 16.f,
												   11.f / 16.f,
												   12.f / 16.f,
												   13.f / 16.f,
												   14.f / 16.f,
												   15.f / 16.f,
												   1.f};
		auto val = gate_widths[gate];
		val += random * Channel::Gate::max;
		val = std::clamp(val, 0.f, 1.f);
		return val;
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
