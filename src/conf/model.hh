#pragma once
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{

constexpr uint32_t NumChans = 8;
constexpr uint32_t NumScenes = 8;
constexpr uint32_t NumBanks = 8;

using OutputBuffer = std::array<uint16_t, NumChans>;

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

static constexpr float max_output_voltage = 10.f;
static constexpr float min_output_voltage = -5.f;

static constexpr unsigned fader_width_mm = 60;

} // namespace Catalyst2::Model
