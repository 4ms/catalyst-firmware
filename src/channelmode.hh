#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "quantizer.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2
{

class ChannelMode {
	static constexpr std::array Scale = {
		Quantizer::Scale{},																// none
		Quantizer::Scale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f}, // chromatic
		Quantizer::Scale{0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f},							// major
		Quantizer::Scale{0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 10.f},							// minor
		Quantizer::Scale{0.f, 2.f, 4.f, 7.f, 9.f},										// major pentatonic
		Quantizer::Scale{0.f, 3.f, 5.f, 7.f, 10.f},										// minor pentatonic
		Quantizer::Scale{0.f, 2.f, 4.f, 8.f, 10.f},										// wholetone
	};

	// one is added for gate mode.
	// unquantized is implicit (see the first scale in the array)
	static constexpr uint8_t Count = Scale.size() + 1;

	// the color that each channel mode is represented by
	static constexpr std::array<Color, Count> color = {
		Color{2, 2, 0},	  // off
		Palette::magenta, // chromatic
		Palette::blue,	  // major
		Palette::red,	  // minor
		Palette::cyan,	  // major pent
		Palette::yellow,  // minor pent
		Palette::pink,	  // wholetone
		Palette::green,	  // gate
	};

	uint8_t val = 0;

public:
	void Inc(int32_t dir) {
		auto temp = val + dir;
		val = std::clamp<int32_t>(temp, 0, Count - 1);
	}

	bool IsGate() {
		return val == Count - 1;
	}

	bool IsQuantized() {
		return !IsGate();
	}

	Quantizer::Scale GetScale() {
		return Scale[IsGate() ? 0 : val];
	}

	Color GetColor() {
		return color[val];
	}
};

} // namespace Catalyst2