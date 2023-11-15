#pragma once

#include "conf/model.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2
{
namespace ChannelValue
{
using type = uint16_t;
static_assert(std::same_as<type, uint16_t>, "ChannelValue::type only tested with uint16_t");

constexpr type Max = UINT16_MAX;
constexpr type Min = 0;
constexpr type Range = Max - Min;

constexpr type from_volts(const float volts) {
	auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
	return MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, Min, Max);
}

constexpr auto GateHigh = from_volts(5.f);
constexpr auto GateSetThreshold = from_volts(0.f);
constexpr auto GateArmed = GateSetThreshold;
constexpr auto GateOff = from_volts(-.1f);

constexpr auto octave = (Range / Model::output_octave_range);
constexpr auto note = octave / 12;

constexpr type inc_step = note + .5f;
constexpr type inc_step_fine = (note / 25.f) + .5f;
} // namespace ChannelValue

struct Channel {
	ChannelValue::type val = ChannelValue::from_volts(0.f);
	void Inc(int32_t inc, bool fine, bool is_gate) {
		if (is_gate) {
			if (inc > 0)
				val = ChannelValue::GateSetThreshold;
			else if (inc < 0)
				val = ChannelValue::GateSetThreshold - 1;
		} else {
			int32_t i = fine ? ChannelValue::inc_step_fine : ChannelValue::inc_step;

			if (inc < 0)
				i *= -1;

			val = std::clamp<int32_t>(val + i, ChannelValue::Min, ChannelValue::Max);
		}
	}
};
} // namespace Catalyst2