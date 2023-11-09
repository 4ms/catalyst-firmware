#pragma once
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{
static constexpr auto SampleRateHz = 3000u;

static constexpr uint32_t NumChans = 8;

static constexpr uint32_t NumScenes = 8;
static constexpr uint32_t NumBanks = 8;

static constexpr auto SeqPages = 8;
static constexpr auto SeqStepsPerPage = 8;
static constexpr auto MaxSeqSteps = SeqPages * SeqStepsPerPage;
static constexpr auto MinSeqSteps = 1;

using OutputBuffer = std::array<uint16_t, NumChans>;

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

static constexpr float max_output_voltage = 10.f;
static constexpr float min_output_voltage = -5.f;

static constexpr float output_octave_range = max_output_voltage - min_output_voltage;

static constexpr unsigned fader_width_mm = 60;

static constexpr unsigned rec_buffer_size = 2048;
static constexpr unsigned rec_buffer_prescaler = 16;

struct EncoderAlts {
	static constexpr auto StartOffset = 0;
	static constexpr auto PlayMode = 1;
	static constexpr auto SeqLength = 2;
	static constexpr auto PhaseOffset = 3;
	static constexpr auto Range = 4;
	static constexpr auto ClockDiv = 5;
	static constexpr auto Transpose = 6;
	static constexpr auto Random = 7;
};
} // namespace Catalyst2::Model
