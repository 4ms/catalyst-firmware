#pragma once
#include "../quantizer.hh"
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{
static constexpr uint32_t NumChans = 8;

static constexpr uint32_t NumScenes = 8;
static constexpr uint32_t NumBanks = 8;

static constexpr uint32_t SeqPages = 8;
static constexpr uint32_t SeqStepsPerPage = 8;
static constexpr uint32_t MaxSeqSteps = SeqPages * SeqStepsPerPage;

using OutputBuffer = std::array<uint16_t, NumChans>;

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

static constexpr float max_output_voltage = 10.f;
static constexpr float min_output_voltage = -5.f;

static constexpr float output_octave_range = max_output_voltage - min_output_voltage;

static constexpr unsigned fader_width_mm = 60;

static constexpr unsigned rec_buffer_size = 2048;
static constexpr unsigned rec_buffer_prescaler = 16;

static constexpr std::array Scale = {
	QuantizerScale{},															  // none
	QuantizerScale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f}, // chromatic
	QuantizerScale{0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f},							  // major
	QuantizerScale{0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 10.f},							  // minor
	QuantizerScale{0.f, 2.f, 4.f, 7.f, 9.f},									  // major pentatonic
	QuantizerScale{0.f, 3.f, 5.f, 7.f, 10.f},									  // minor pentatonic
	QuantizerScale{0.f, 2.f, 4.f, 8.f, 10.f},									  // wholetone
};

// one is added for gate mode.
// unquantized is implicit (see the first scale in the array)
static constexpr uint8_t ChannelModeCount = Scale.size() + 1;

struct EncoderAlts {
	static constexpr auto StartOffset = 0;
	static constexpr auto PlayMode = 1;
	static constexpr auto SeqLength = 2;
	static constexpr auto ClockDiv = 5;
	static constexpr auto Random = 7;
};
} // namespace Catalyst2::Model
