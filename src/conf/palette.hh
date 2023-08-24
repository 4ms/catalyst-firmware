#pragma once
#include "util/colors.hh"

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
	static constexpr Color yellow = Color(127, 0, 127);
	static constexpr Color green = Color(0, 0, 255);
	static constexpr Color cyan = Color(0, 255, 255);
	static constexpr Color blue = Color(0, 255, 0);
	static constexpr Color purple = Color(255, 255, 0);
	static constexpr Color magenta = Color(200, 100, 0);

	static Color from_raw(int val)
	{
		return Color(val, (val >> 8), (val >> 16));
	}
};

} // namespace Catalyst2
