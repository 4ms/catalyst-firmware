#pragma once

#include "../../lib/cpputil/util/colors.hh"
#include "channel.hh"
#include "model.hh"
#include "range.hh"
#include "sequencer_step.hh"
#include <array>
#include <cstdint>
#include <type_traits>

namespace Catalyst2::Palette
{
// Board is Red-Blue-Green (not normal RGB)
inline constexpr Color off = Color(0, 0, 0);
inline constexpr Color black = Color(0, 0, 0);
inline constexpr Color very_dim_grey = Color(3, 3, 3);
inline constexpr Color dim_grey = Color(5, 5, 5);
inline constexpr Color grey = Color(100, 40, 40);

inline constexpr Color red = Color(128, 0, 0);
inline constexpr Color pink = Color(150, 70, 20);
inline constexpr Color orange = Color(250, 0, 25);
inline constexpr Color yellow = Color(80, 0, 30);
inline constexpr Color dim_green = Color(0, 0, 15);
inline constexpr Color green = Color(0, 0, 90);
inline constexpr Color cyan = Color(0, 90, 90);
inline constexpr Color blue = Color(0, 128, 0);
inline constexpr Color magenta = Color(100, 50, 0);

inline constexpr Color full_white = Color(255, 255, 255);
inline constexpr Color full_red = Color(255, 0, 0);
inline constexpr Color full_green = Color(0, 0, 255);
inline constexpr Color full_blue = Color(0, 255, 0);

namespace Voltage
{
inline constexpr auto Negative = full_red;
inline constexpr auto Positive = blue;
} // namespace Voltage

namespace Setting
{
inline constexpr auto null = very_dim_grey;
inline constexpr auto active = blue;
inline constexpr auto playmode_fwd = blue;
inline constexpr auto playmode_bck = red;
inline constexpr auto slider_slew = pink;
inline constexpr auto curve_linear = grey.blend(off, 0.3f);
inline constexpr auto curve_expo = yellow;

namespace OutputOverride
{
inline constexpr auto on = green;
inline constexpr auto off = red;
} // namespace OutputOverride

namespace Transpose
{
inline constexpr auto positive = blue;
inline constexpr auto negative = red;
} // namespace Transpose

namespace ClockDiv
{
inline constexpr std::array color = {
	blue,
	pink,
	grey,
	orange,
};
}
inline constexpr auto bpm = yellow;
} // namespace Setting

namespace Scales
{
inline constexpr std::array color = {
	very_dim_grey,			 // off
	pink,					 // chromatic
	red.blend(off, 0.5f),	 // major
	yellow.blend(off, 0.5f), // minor
	cyan.blend(off, 0.5f),	 // major pent
	blue.blend(off, 0.5f),	 // minor pent
	grey,					 // wholetone
	green.blend(off, 0.5f),	 // gate
};
}

namespace SeqHead
{
inline constexpr auto color = magenta;
}

namespace Morph
{
inline constexpr Color color(float phase) {
	phase = 1.f - phase;
	constexpr auto linear = grey.blend(off, 0.3f);
	constexpr auto chop = red.blend(off, 0.3f);
	return linear.blend(chop, phase);
}
} // namespace Morph

namespace Probability
{
inline constexpr Color color(Catalyst2::Sequencer::Probability::type p) {
	const auto phase = Catalyst2::Sequencer::Probability::toFloat(p);
	if (phase < 0.5f)
		return off.blend(orange, phase * 2.f);
	else
		return orange.blend(green, phase * 2.f - 1.f);
}
} // namespace Probability

namespace Pathway
{
inline constexpr Color color(float phase) {
	constexpr auto first_page = green.blend(off, 0.5f);
	constexpr auto last_page = red.blend(off, 0.5f);
	return first_page.blend(last_page, phase);
}
} // namespace Pathway

namespace Random
{
inline constexpr auto none = off;
inline constexpr auto set = red;

inline constexpr Color color(uint8_t val) {
	uint8_t r = (val & 0b0110000) >> 1;
	uint8_t b = (val & 0b0001100) << 1;
	uint8_t g = (val & 0b0000011) << 3;
	if (!r && !b && !g)
		return grey;

	return Color(r, b, g);
}
} // namespace Random

namespace Cv
{
namespace Details
{
inline Color pimpl(int level, uint16_t zero) {
	level -= zero;
	const auto color = level < 0 ? Voltage::Negative : Voltage::Positive;
	const auto phase = (std::abs(level) / static_cast<float>(zero));
	return off.blend(color, phase);
}
} // namespace Details
inline Color fromLevel(Channel::Cv::type level, Channel::Cv::Range range) {
	level = std::clamp<int32_t>(level, RangeToMin(range), RangeToMax(range));
	return Details::pimpl(level, Channel::Cv::zero);
}

inline Color fromOutput(Model::Output::type level) {
	return Details::pimpl(level, Channel::Output::from_volts(0.f));
}
} // namespace Cv

namespace Gate
{
inline constexpr auto color = dim_green;
inline constexpr auto max = cyan;
inline Color fromLevel(Channel::Gate::type level) {
	return level >= 1.f ? max : off.blend(color, level);
}

inline Color fromOutput(Model::Output::type level) {
	return level == Channel::Output::gate_off ? off : color;
}

inline Color fromTrigDelay(float val) {
	const auto col = val < 0.f ? Voltage::Negative : Voltage::Positive;
	val *= val < 0.f ? -1.f : 1.f;
	return off.blend(col, val);
}
} // namespace Gate

inline constexpr Color ManualRGB(Model::Output::type r, Model::Output::type g, Model::Output::type b) {
	float R = (float)r / (float)Channel::Output::max;
	float G = (float)g / (float)Channel::Output::max;
	float B = (float)b / (float)Channel::Output::max;

	auto true_red = Color{255, 0, 0};
	auto true_blue = Color{0, 0, 255};
	auto true_green = Color{0, 255, 0};
	// Note: since Catalyst uses RBG but Color is RGB, the meanings for Color::blue() and Color::green() are swapped
	return Color{off.blend(true_red, R).red(), off.blend(true_blue, B).blue(), off.blend(true_green, G).green()};
}

static_assert(ManualRGB(16384, 32768, 49152).red() == 63);
static_assert(ManualRGB(16384, 32768, 49152).blue() == 127);
static_assert(ManualRGB(16384, 32768, 49152).green() == 191);

} // namespace Catalyst2::Palette
