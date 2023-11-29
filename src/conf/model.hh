#pragma once

#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{
constexpr auto sample_rate_hz = 3000u;
constexpr auto clock_mult_factor = 12u;

constexpr auto triglengthms = 5u;

constexpr auto NumChans = 8u;
constexpr auto NumScenes = 8u;
constexpr auto NumBanks = 8u;

constexpr auto SeqStepsPerPage = NumChans;
constexpr auto SeqPages = NumScenes;
constexpr auto MaxSeqSteps = SeqPages * SeqStepsPerPage;
constexpr auto MinSeqSteps = 1;

namespace Output
{
using type = uint16_t;
static_assert(std::same_as<type, uint16_t>, "type on tested with uint16_t");
using Buffer = std::array<type, NumChans>;
} // namespace Output

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

constexpr auto max_output_voltage = 10.f;
constexpr auto min_output_voltage = -5.f;
constexpr auto output_octave_range = max_output_voltage - min_output_voltage;

constexpr auto fader_width_mm = 60u;

constexpr auto rec_buffer_size = 2048u;
constexpr auto rec_buffer_prescaler = 16u;

struct EncoderAlts {
	static constexpr auto StartOffset = 0u;
	static constexpr auto PlayMode = 1u;
	static constexpr auto SeqLength = 2u;
	static constexpr auto PhaseOffset = 3u;
	static constexpr auto Range = 4u;
	static constexpr auto ClockDiv = 5u;
	static constexpr auto Transpose = 6u;
	static constexpr auto Random = 7u;
};
} // namespace Catalyst2::Model
