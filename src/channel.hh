#pragma once

#include "conf/model.hh"
#include "random.hh"
#include "range.hh"
#include <algorithm>
#include <cstdint>
#include <numeric>

namespace Catalyst2::Channel
{

constexpr auto gatehigh = from_volts(5.f);
constexpr auto gatearmed = from_volts(0.f) + 1u;
constexpr auto gateoff = gatearmed - 1u;

constexpr auto octave = range / Model::output_octave_range;
constexpr auto note = octave / 12;

class Value {
	static constexpr auto inc_step_fine = 1;
	static constexpr auto inc_step = inc_step_fine * 25;
	static constexpr auto min = 0, max = static_cast<int>(inc_step * 12 * Model::output_octave_range);
	static constexpr auto zero =
		MathTools::map_value(0.f, Model::min_output_voltage, Model::max_output_voltage, min, max);
	static constexpr auto mid_point = max / 2;
	static_assert(min == 0, "mid_point will be wrong if min != 0");
	int16_t val;

public:
	class Proxy {
		const int32_t val;

	public:
		Proxy(int32_t offset)
			: val{offset} {
		}
		Model::Output::type AsCV() const {
			return val / static_cast<float>(max) * Channel::max;
		}
		bool AsGate() const {
			return val & 0x01;
		}
	};

	constexpr Value(float volts = 0.f) {
		auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
		val = MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, min, max);
	}
	void IncCv(int32_t inc, bool fine, Range range) {
		inc *= fine ? inc_step_fine : inc_step;
		const auto min_ = MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
		const auto max_ = MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
		val = std::clamp<int32_t>(val + inc, min_, max_);
	}
	void IncGate(int32_t inc) {
		val = std::clamp<int32_t>(val + inc, mid_point, max);
	}
	Proxy Read(Range range, float random) const {
		const auto t = val + (random * max);
		const auto min_ = MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
		const auto max_ = MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
		return Proxy{std::clamp<int32_t>(t, min_, max_)};
	}
	bool Validate() const {
		return val >= min && val <= max;
	}
};
} // namespace Catalyst2::Channel
