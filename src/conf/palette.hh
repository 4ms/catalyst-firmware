#pragma once

#include "../../lib/cpputil/util/colors.hh"
#include "channel.hh"
#include "model.hh"
#include <array>
#include <cstdint>

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
inline constexpr Color orange = Color(150, 0, 100);
inline constexpr Color yellow = Color(60, 0, 60);
inline constexpr Color dim_green = Color(0, 0, 9);
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

namespace Gate
{
inline constexpr auto color = dim_green;
}

namespace Setting
{
inline constexpr auto null = very_dim_grey;
inline constexpr auto active = blue;
inline constexpr auto playmode_fwd = blue;
inline constexpr auto playmode_bck = red;
inline constexpr auto slider_slew = pink;
inline constexpr auto curve_linear = grey.blend(off, 0.3f);
inline constexpr auto curve_expo = yellow;

namespace Transpose
{
inline constexpr auto positive = blue;
inline constexpr auto negative = red;
} // namespace Transpose

namespace ClockDiv
{
inline constexpr std::array color = {blue, pink, grey, orange};
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
	constexpr auto linear = grey.blend(off, 0.3f);
	constexpr auto chop = red.blend(off, 0.3f);
	return linear.blend(chop, phase);
}
} // namespace Morph

namespace Probability
{
inline constexpr Color color(float phase) {
	return off.blend(green.blend(off, .5f), phase);
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

inline constexpr Color TrigDelayBlend(float val) {
	const auto col = val < 0.f ? Voltage::Negative : Voltage::Positive;
	val *= val < 0.f ? -1.f : 1.f;
	return off.blend(col, val);
}

inline constexpr Color CvBlend(uint16_t level) {
	using namespace Channel;
	constexpr auto neg = from_volts(0.f);
	auto temp = level - neg;
	auto c = Voltage::Positive;
	if (temp < 0) {
		temp *= -2;
		c = Voltage::Negative;
	}
	const auto phase = (temp / (neg * 2.f));
	return off.blend(c, phase);
}

inline constexpr Color GateBlend(uint16_t level) {
	if (level == UINT16_MAX) {
		return cyan;
	}
	return off.blend(green, static_cast<uint8_t>(level >> 8));
}

inline constexpr Color EncoderBlend(Model::Output::type val, bool isgate) {
	return isgate ? GateBlend(val >= Channel::gatearmed) : CvBlend(val);
}

} // namespace Catalyst2::Palette
