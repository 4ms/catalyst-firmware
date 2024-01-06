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
constexpr Color dim_grey = Color(5, 5, 5);
constexpr Color grey = Color(100, 40, 40);

constexpr Color red = Color(128, 0, 0);
constexpr Color pink = Color(80, 20, 20);
constexpr Color orange = Color(150, 0, 100);
constexpr Color yellow = Color(150, 0, 60);
constexpr Color green = Color(0, 0, 90);
constexpr Color cyan = Color(0, 90, 90);
constexpr Color blue = Color(0, 128, 0);
constexpr Color magenta = Color(100, 50, 0);

constexpr Color full_white = Color(255, 255, 255);
constexpr Color full_red = Color(255, 0, 0);
constexpr Color full_green = Color(0, 0, 255);
constexpr Color full_blue = Color(0, 255, 0);

struct Voltage {
	static constexpr auto Negative = full_red;
	static constexpr auto Positive = blue;
};

struct Gate {
	static constexpr auto Primed = Color{0, 0, 8};
};

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
	Color{4, 4, 4},					  // off
	Palette::pink,					  // chromatic
	Palette::red.blend(off, 0.5f),	  // major
	Palette::yellow.blend(off, 0.5f), // minor
	Palette::cyan.blend(off, 0.5f),	  // major pent
	Palette::blue.blend(off, 0.5f),	  // minor pent
	Palette::grey,					  // wholetone
	Palette::green.blend(off, 0.5f),  // gate
};
}

constexpr auto seqhead = magenta;

namespace Morph
{
constexpr auto linear = Palette::grey;
constexpr auto chop = Palette::red;
constexpr Color color(float phase) {
	return linear.blend(chop, phase);
}
} // namespace Morph

namespace Pathway
{
constexpr Color color(float phase) {
	return green.blend(red, phase);
}
} // namespace Pathway

namespace Random
{
constexpr auto none = off;

constexpr Color color(uint8_t val) {
	uint8_t r = val & 0xc0;
	uint8_t b = (val << 2) & 0xc0;
	uint8_t g = (val << 4) & 0xc0;
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
	return level == true ? Gate::Primed : off;
}

constexpr Color EncoderBlend(Model::Output::type val, bool isgate) {
	return isgate ? GateBlend(val >= Channel::gatearmed) : CvBlend(val);
}

} // namespace Catalyst2::Palette
