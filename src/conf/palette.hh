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
inline constexpr Color pink = Color(125, 25, 7);
inline constexpr Color orange = Color(250, 0, 25);
inline constexpr Color yellow = Color(80, 0, 30);
inline constexpr Color dim_green = Color(0, 0, 31);
inline constexpr Color green = Color(0, 0, 90);
inline constexpr Color cyan = Color(0, 90, 90);
inline constexpr Color teal = Color(0, 32, 90);
inline constexpr Color blue = Color(0, 128, 0);
inline constexpr Color magenta = Color(100, 50, 0);
inline constexpr Color salmon = Color(153, 5, 13);
inline constexpr Color lavender = Color(100, 100, 0);

inline constexpr Color bright_red = Color(180, 0, 0);

inline constexpr Color full_white = Color(255, 255, 255);
inline constexpr Color full_red = Color(255, 0, 0);
inline constexpr Color full_green = Color(0, 0, 255);
inline constexpr Color full_blue = Color(0, 255, 0);

namespace Voltage
{
inline constexpr auto Negative = full_red;
inline constexpr auto Positive = blue;
} // namespace Voltage

namespace Range
{
inline constexpr auto Negative = full_red;
inline constexpr std::array Positive = {blue, cyan, green, yellow, orange, magenta};

constexpr Color color(Channel::Cv::Range range) {
	return Positive[range.Index()];
}
} // namespace Range

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
	very_dim_grey,			   // off
	pink,					   // chromatic
	red.blend(off, 0.5f),	   // major
	orange.blend(off, 0.5f),   // minor
	yellow.blend(off, 0.5f),   // harm. minor
	teal.blend(off, 0.5f),	   // major pent
	blue.blend(off, 0.5f),	   // minor pent
	grey,					   // wholetone
	salmon.blend(off, 0.5f),   // lydian dom.
	lavender.blend(off, 0.5f), // Beebop
	red.blend(off, 0.9f),	   // enigmatic
	yellow.blend(off, 0.9f),   // vietnamese
	orange.blend(off, 0.9f),   // Yo scale
	blue.blend(off, 0.95f),	   // 16-TET
	teal.blend(off, 0.95f),	   // 21-TET

	green.blend(off, 0.5f), // gate
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
		return off.blend(orange.blend(off, 0.65f), phase * 2.f);
	else
		return orange.blend(teal.blend(off, 0.5f), phase * 2.f - 1.f);
}
} // namespace Probability

namespace Pathway
{
inline constexpr Color color(float phase) {
	constexpr auto first_page = green.blend(off, 0.5f);
	constexpr auto last_page = red.blend(off, 0.5f);

	phase -= 1.f / 64.f;
	auto p = static_cast<int>(phase * 8.f);
	phase = std::clamp(p / 8.f, 0.f, 1.f);
	phase = 1.f - (1.f - phase) * (1.f - phase);
	return first_page.blend(last_page, phase);
}
} // namespace Pathway

namespace Random
{
inline constexpr auto none = off;
inline constexpr auto set = bright_red;

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

inline Color fromOutput(Model::Output::type out_level) {
	constexpr auto zero = Channel::Output::from_volts(0.f);
	int level = out_level - zero;
	const auto color = level < 0 ? Voltage::Negative : Voltage::Positive;
	auto phase = level / static_cast<float>(zero);
	phase *= level < 0 ? -1.f : 0.5f;
	return off.blend(color, phase);
}

inline Color fromLevel(Channel::Cv::type level, Channel::Cv::Range range) {
	auto out = Channel::Output::Scale(level, range.Min(), range.Max());
	return fromOutput(out);
}

} // namespace Cv

namespace Gate
{
inline constexpr auto color = dim_green;
inline constexpr auto max = teal;
inline Color fromLevelSequencer(Channel::Gate::type level) {
	if (level >= 1.f)
		return max;
	if (level == 0.f)
		return off;
	constexpr uint8_t max_element = std::max(color.red(), std::max(color.green(), color.blue()));
	if (level < 1.f / max_element)
		level = 1.f / max_element;
	return off.blend(color, level);
}

inline Color fromLevelMacro(Channel::Gate::type level) {
	return off.blend(color, level);
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
	r = std::clamp(r, Channel::Cv::zero, Channel::Cv::max);
	g = std::clamp(g, Channel::Cv::zero, Channel::Cv::max);
	b = std::clamp(b, Channel::Cv::zero, Channel::Cv::max);
	float R = (float)(r - Channel::Cv::zero) / (float)(Channel::Cv::max - Channel::Cv::zero);
	float G = (float)(g - Channel::Cv::zero) / (float)(Channel::Cv::max - Channel::Cv::zero);
	float B = (float)(b - Channel::Cv::zero) / (float)(Channel::Cv::max - Channel::Cv::zero);

	auto true_red = Color{255, 0, 0};
	auto true_blue = Color{0, 0, 255};
	auto true_green = Color{0, 255, 0};
	// Note: since Catalyst uses RBG but Color is RGB, the meanings for Color::blue() and Color::green() are swapped
	return Color{off.blend(true_red, R).red(), off.blend(true_blue, B).blue(), off.blend(true_green, G).green()};
}

// Tests:
namespace
{
using namespace Channel::Cv;
inline constexpr auto mid = (max + zero) / 2.f;
static_assert(ManualRGB(0, mid, mid).red() == 0);
static_assert(ManualRGB(zero, mid, mid).red() == 0);
static_assert(ManualRGB(mid, 0, 0).red() == 127);
static_assert(ManualRGB(max, 0, 0).red() == 255);

static_assert(ManualRGB(mid, mid, 0).green() == 0);
static_assert(ManualRGB(mid, mid, zero).green() == 0);
static_assert(ManualRGB(0, 0, mid).green() == 127);
static_assert(ManualRGB(0, 0, max).green() == 255);

static_assert(ManualRGB(mid, 0, mid).blue() == 0);
static_assert(ManualRGB(mid, zero, mid).blue() == 0);
static_assert(ManualRGB(0, mid, 0).blue() == 127);
static_assert(ManualRGB(0, max, 0).blue() == 255);
} // namespace

} // namespace Catalyst2::Palette
