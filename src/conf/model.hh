#pragma once

#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{
enum class Mode : uint8_t { Sequencer, Macro };

inline constexpr auto NumChans = 8u;
inline constexpr auto sample_rate_hz = 4000u;

inline constexpr auto max_output_voltage = 10.f;
inline constexpr auto min_output_voltage = -5.f;
inline constexpr auto output_octave_range = max_output_voltage - min_output_voltage;

inline constexpr auto fader_width_mm = 60u;

namespace HoldTimes
{
inline constexpr auto mode_switcher = 3000u;
inline constexpr auto reset = 3000u;
inline constexpr auto save = 3000u;
} // namespace HoldTimes

namespace Sequencer
{
inline constexpr auto NumSlots = 8u;
inline constexpr auto NumPages = NumChans;

inline constexpr auto MaxQueuedStartOffsetPages = 64u;

namespace Steps
{
inline constexpr auto PerPage = NumChans;
inline constexpr auto Max = NumPages * PerPage;
inline constexpr auto Min = 1;
} // namespace Steps

namespace EncoderAlts
{
inline constexpr auto StartOffset = 0u;
inline constexpr auto PlayMode = 1u;
inline constexpr auto SeqLength = 2u;
inline constexpr auto PhaseOffset = 3u;
inline constexpr auto Range = 4u;
inline constexpr auto ClockDiv = 5u;
inline constexpr auto Transpose = 6u;
inline constexpr auto Random = 7u;
}; // namespace EncoderAlts
} // namespace Sequencer

namespace Macro
{
inline constexpr auto NumScenes = NumChans;

namespace Bank
{
inline constexpr auto NumNormal = NumChans;
inline constexpr auto NumClassic = 1u;
inline constexpr auto NumTotal = NumNormal + NumClassic;
} // namespace Bank

inline constexpr auto rec_buffer_size = 2048u;
inline constexpr auto rec_buffer_prescaler = 16u;

namespace EncoderAlts
{
inline constexpr auto OutputOverride = 0u;
inline constexpr auto SliderSlewCurve = 4u;
inline constexpr auto ClockDiv = 5u;
inline constexpr auto SliderSlew = 6u;
inline constexpr auto Random = 7u;
}; // namespace EncoderAlts
} // namespace Macro

namespace Output
{
using type = uint16_t;
static_assert(std::same_as<type, uint16_t>, "type only tested with uint16_t");
using Buffer = std::array<type, NumChans>;
} // namespace Output

enum class AdcElement { Slider, CVJack };

} // namespace Catalyst2::Model
