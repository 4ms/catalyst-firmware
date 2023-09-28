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

static constexpr type Max = UINT16_MAX;
static constexpr type Min = 0;
static constexpr type Range = Max - Min;

static constexpr type from_volts(const float volts)
{
	auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
	return MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, Min, Max);
}

static constexpr type GateHigh = from_volts(5.f);
static constexpr type GateSetFlag = from_volts(.1f);
static constexpr type GateOffFlag = from_volts(.0f);

static constexpr type inc_step = (Range / Model::output_octave_range / 12.f) + .5f;
static constexpr type inc_step_fine = (Range / Model::output_octave_range / 12.f / 25.f) + .5f;

static constexpr type Inc(type val, int32_t dir, bool fine, bool is_gate)
{
	if (is_gate) {
		if (dir > 0)
			return GateSetFlag;
		else if (dir < 0)
			return GateOffFlag;
	} else {
		if (dir > 0)
			dir = fine ? inc_step_fine : inc_step;
		else if (dir < 0)
			dir = fine ? inc_step_fine * -1 : inc_step * -1;
		return std::clamp<int32_t>(val + dir, 0, Max);
	}
}
} // namespace ChannelValue
struct Channel {
	ChannelValue::type val = ChannelValue::from_volts(0.f);
	void Inc(int32_t dir, bool fine, bool is_gate)
	{
		if (is_gate) {
			if (dir > 0)
				val = ChannelValue::GateSetFlag;
			else if (dir < 0)
				val = ChannelValue::GateOffFlag;
		} else {
			int32_t inc = fine ? ChannelValue::inc_step_fine : ChannelValue::inc_step;

			if (dir < 0)
				inc *= -1;

			val = std::clamp<int32_t>(val + inc, ChannelValue::Min, ChannelValue::Max);
		}
	}
};
} // namespace Catalyst2