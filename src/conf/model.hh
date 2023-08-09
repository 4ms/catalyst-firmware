#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{

constexpr uint32_t NumChans = 8;
constexpr uint32_t NumBanks = 8;

using OutputBuffer = std::array<uint16_t, NumChans>;

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

static constexpr float max_output_voltage = 10.f;
static constexpr float min_output_voltage = -5.f;

constexpr uint16_t volts_to_uint(const float volts)
{
	auto temp = std::clamp(volts, min_output_voltage, max_output_voltage);
	temp -= min_output_voltage;
	temp /= max_output_voltage - min_output_voltage;
	return static_cast<uint16_t>(temp * 65535u);
}

} // namespace Catalyst2::Model
