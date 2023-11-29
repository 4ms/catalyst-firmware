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
constexpr Color grey = Color(127, 127, 127);
constexpr Color white = Color(255, 255, 255);
constexpr Color red = Color(255, 0, 0);
constexpr Color pink = Color(255, 0xb4, 0x69);
constexpr Color pale_pink = Color(255, 200, 200);
constexpr Color tangerine = Color(255, 0, 200);
constexpr Color orange = Color(255, 0, 127);
constexpr Color yellow = Color(191, 0, 63);
constexpr Color green = Color(0, 0, 255);
constexpr Color cyan = Color(0, 255, 255);
constexpr Color blue = Color(0, 255, 0);
constexpr Color purple = Color(255, 255, 0);
constexpr Color magenta = Color(200, 100, 0);

struct Voltage {
	static constexpr auto Negative = red;
	static constexpr auto Positive = blue;
};

struct Gate {
	static constexpr auto Primed = Color{0, 0, 2};
	static constexpr auto High = green;
};

struct Setting {
	static constexpr auto null = Color{10, 10, 10};
	static constexpr auto active = blue;
};

constexpr auto seqhead = magenta;

constexpr auto bpm = yellow;

// filter out red colors
constexpr Color from_raw(int8_t val) {
	const uint8_t r = val & 0xc0;
	uint8_t b = (val << 2) & 0xc0;
	uint8_t g = (val << 4) & 0xc0;
	if (!b && !g) {
		b = r;
		g = ~r;
	}
	return Color(r, b, g);
}

constexpr Color EncoderBlend(uint16_t level, bool is_gate) {
	using namespace Channel;
	if (is_gate) {
		if (level == gatearmed)
			return Gate::Primed;
		if (level == gatehigh)
			return Gate::High;
		return off;
	} else {
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
}

} // namespace Catalyst2::Palette
