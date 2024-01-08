#pragma once

#include "../../lib/cpputil/util/colors.hh"
#include "channel.hh"
#include "model.hh"
#include <array>

namespace Catalyst2::Palette
{
// Board is Red-Blue-Green (not normal RGB)
constexpr Color off = Color(0, 0, 0);
constexpr Color black = Color(0, 0, 0);
constexpr Color very_dim_grey = Color(3, 3, 3);
constexpr Color dim_grey = Color(5, 5, 5);
constexpr Color grey = Color(100, 40, 40);

constexpr Color red = Color(128, 0, 0);
constexpr Color pink = Color(80, 20, 20);
constexpr Color orange = Color(150, 0, 100);
constexpr Color yellow = Color(150, 0, 60);
constexpr Color dim_green = Color(0, 0, 9);
constexpr Color green = Color(0, 0, 90);
constexpr Color cyan = Color(0, 90, 90);
constexpr Color blue = Color(0, 128, 0);
constexpr Color magenta = Color(100, 50, 0);

constexpr Color full_white = Color(255, 255, 255);
constexpr Color full_red = Color(255, 0, 0);
constexpr Color full_green = Color(0, 0, 255);
constexpr Color full_blue = Color(0, 255, 0);

namespace Voltage
{
constexpr auto Negative = full_red;
constexpr auto Positive = blue;
} // namespace Voltage

namespace Gate
{
constexpr auto color = dim_green;
}

namespace Setting
{
constexpr auto null = dim_grey;
constexpr auto active = blue;
constexpr auto playmode_fwd = blue;
constexpr auto playmode_bck = red;
constexpr auto transpose = green;
constexpr auto clockdiv = blue;
constexpr auto bpm = yellow;
} // namespace Setting

namespace Scales
{
constexpr inline std::array color = {
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
constexpr auto color = magenta;
}

namespace Morph
{
constexpr Color color(float phase) {
	constexpr auto linear = grey.blend(off, 0.3f);
	constexpr auto chop = red.blend(off, 0.3f);
	return linear.blend(chop, phase);
}
} // namespace Morph

namespace Pathway
{
constexpr Color color(float phase) {
	constexpr auto first_page = green.blend(off, 0.5f);
	constexpr auto last_page = red.blend(off, 0.5f);
	return first_page.blend(last_page, phase);
}
} // namespace Pathway

namespace Random
{
constexpr auto none = off;
constexpr auto set = red;

constexpr Color color(uint8_t val) {
	uint8_t r = (val & 0b0110000) >> 1;
	uint8_t b = (val & 0b0001100) << 1;
	uint8_t g = (val & 0b0000011) << 3;
	if (!r && !b && !g)
		return grey;

	return Color(r, b, g);
}
} // namespace Random

constexpr Color CvBlend(uint16_t level) {
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

constexpr Color GateBlend(bool level) {
	return level == true ? Gate::color : off;
}

constexpr Color EncoderBlend(Model::Output::type val, bool isgate) {
	return isgate ? GateBlend(val >= Channel::gatearmed) : CvBlend(val);
}

} // namespace Catalyst2::Palette
