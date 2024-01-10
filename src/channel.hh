#pragma once

#include "conf/model.hh"
#include "random.hh"
#include "range.hh"
#include <algorithm>
#include <cstdint>

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
		float AsGate() const {
			if (val < mid_point) {
				return 0.f;
			} else {
				return (val - mid_point) / static_cast<float>(mid_point);
			}
		}
	};
	constexpr Value(float volts = 0.f) {
		auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
		val = MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, min, max);
	}
	void Inc(int32_t inc, bool fine, bool is_gate, Range range, Random::Pool::type offset) {
		inc *= fine ? inc_step_fine : inc_step;

		const auto o = CalculateRandom(range, offset);
		const auto min_ = MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
		const auto max_ = MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
		val = std::clamp<int32_t>(val + inc, min_ - o, max_ - o);
		if (is_gate && val < mid_point - o) {
			val = mid_point - o;
		}
	}
	Proxy Read(Range range, Random::Pool::type offset) const {
		const auto t = val + CalculateRandom(range, offset);
		const auto min_ = MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
		const auto max_ = MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
		return Proxy{std::clamp<int32_t>(t, min_, max_)};
	}
	bool Validate() const {
		return val >= min && val <= max;
	}

private:
	int32_t CalculateRandom(Range range, Random::Pool::type offset) const {
		const auto pa = range.PosAmount();
		auto na = range.NegAmount();
		na = na == 0.f ? -pa : na;
		offset *= offset < 0.f ? na : pa;
		return offset * max;
	}
};
} // namespace Catalyst2::Channel