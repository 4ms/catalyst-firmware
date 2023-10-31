#pragma once
#include "../../lib/cpputil/util/colors.hh"
#include "../channelvalue.hh"
#include "model.hh"
#include <array>

namespace Catalyst2
{
struct Palette {
	// Board is Red-Blue-Green (not normal RGB)
	static constexpr Color off = Color(0, 0, 0);
	static constexpr Color black = Color(0, 0, 0);
	static constexpr Color grey = Color(127, 127, 127);
	static constexpr Color white = Color(255, 255, 255);

	static constexpr Color red = Color(255, 0, 0);
	static constexpr Color pink = Color(255, 0xb4, 0x69);
	static constexpr Color pale_pink = Color(255, 200, 200);
	static constexpr Color tangerine = Color(255, 0, 200);
	static constexpr Color orange = Color(255, 0, 127);
	static constexpr Color yellow = Color(191, 0, 63);
	static constexpr Color green = Color(0, 0, 255);
	static constexpr Color cyan = Color(0, 255, 255);
	static constexpr Color blue = Color(0, 255, 0);
	static constexpr Color purple = Color(255, 255, 0);
	static constexpr Color magenta = Color(200, 100, 0);

	struct Voltage {
		static constexpr auto Negative = red;
		static constexpr auto Positive = blue;
	};

	struct Gate {
		static constexpr auto Primed = Color{0, 0, 2};
		static constexpr auto High = green;
	};

	// the color that each channel mode is represented by
	static constexpr std::array<Color, Model::ChannelModeCount> ChannelMode = {
		Color{2, 2, 0}, // off
		magenta,		// chromatic
		blue,			// major
		red,			// minor
		cyan,			// major pent
		yellow,			// minor pent
		pink,			// wholetone
		green,			// gate
	};

	static constexpr auto seqhead = magenta;

	static constexpr auto globalsetting = red;

	static constexpr auto bpm = yellow;

	// filter out red colors
	static Color from_raw(int8_t val) {
		const uint8_t r = val & 0xc0;
		uint8_t b = (val << 2) & 0xc0;
		uint8_t g = (val << 4) & 0xc0;
		if (!b && !g) {
			b = r;
			g = ~r;
		}
		return Color(r, b, g);
	}

	static Color EncoderBlend(uint16_t level, bool is_gate) {
		if (is_gate) {
			if (level >= ChannelValue::GateSetThreshold)
				return Gate::Primed;
			if (level == ChannelValue::GateHigh)
				return Gate::High;
			return off;
		} else {
			constexpr auto neg = ChannelValue::from_volts(0.f);
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
};

} // namespace Catalyst2
