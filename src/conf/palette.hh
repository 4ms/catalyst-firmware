#pragma once
#include "../../lib/cpputil/util/colors.hh"
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

	static Color from_raw(int8_t val)
	{
		return Color(val & 0xC0, (val << 2) & 0xC0, (val << 4) & 0xC0);
	}
};

} // namespace Catalyst2
