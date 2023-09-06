#pragma once
#include "scales.hh"
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

static constexpr float output_octave_range = max_output_voltage - min_output_voltage;

static constexpr unsigned fader_width_mm = 60;

static constexpr unsigned rec_buffer_size = 2048;
static constexpr unsigned rec_buffer_prescaler = 16;

// quantizer scales
// static constexpr Scales::Scale custom_example = {99.f, 22.f};
static constexpr std::array scales = {
	Scales::chromatic, Scales::major, Scales::minor,
	//	custom_example,
};
} // namespace Catalyst2::Model
