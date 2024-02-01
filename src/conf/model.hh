#pragma once

#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{
enum class Mode : bool { Sequencer, Macro };

inline constexpr auto default_mode = Mode::Sequencer;

inline constexpr auto sample_rate_hz = 3000u;
inline constexpr auto clock_mult_factor = 12u;

inline constexpr auto triglengthms = 5u;

inline constexpr auto NumChans = 8u;
inline constexpr auto NumScenes = 8u;
inline constexpr auto NumBanks = 8u;

inline constexpr auto SeqStepsPerPage = NumChans;
inline constexpr auto SeqPages = NumScenes;
inline constexpr auto MaxSeqSteps = SeqPages * SeqStepsPerPage;
inline constexpr auto MinSeqSteps = 1;
inline constexpr auto MaxQueuedStartOffsetPages = 16u;

inline constexpr auto max_queued_start_offset_changes = 8u;

namespace Output
{
using type = uint16_t;
static_assert(std::same_as<type, uint16_t>, "type only tested with uint16_t");
using Buffer = std::array<type, NumChans>;
} // namespace Output

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

inline constexpr auto max_output_voltage = 10.f;
inline constexpr auto min_output_voltage = -5.f;
inline constexpr auto output_octave_range = max_output_voltage - min_output_voltage;

inline constexpr auto fader_width_mm = 60u;

inline constexpr auto rec_buffer_size = 2048u;
inline constexpr auto rec_buffer_prescaler = 16u;

struct SeqEncoderAlts {
	static constexpr auto StartOffset = 0u;
	static constexpr auto PlayMode = 1u;
	static constexpr auto SeqLength = 2u;
	static constexpr auto PhaseOffset = 3u;
	static constexpr auto Range = 4u;
	static constexpr auto ClockDiv = 5u;
	static constexpr auto Transpose = 6u;
	static constexpr auto Random = 7u;
};
struct MacroEncoderAlts {
	static constexpr auto SliderSlewCurve = 4u;
	static constexpr auto ClockDiv = 5u;
	static constexpr auto SliderSlew = 6u;
	static constexpr auto Random = 7u;
};
} // namespace Catalyst2::Model
