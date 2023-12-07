#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "quantizer.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Channel
{

class Mode {
	static constexpr std::array Scale = {
		Quantizer::Scale{},																// none
		Quantizer::Scale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f}, // chromatic
		Quantizer::Scale{0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f},							// major
		Quantizer::Scale{0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 10.f},							// minor
		Quantizer::Scale{0.f, 2.f, 4.f, 7.f, 9.f},										// major pentatonic
		Quantizer::Scale{0.f, 3.f, 5.f, 7.f, 10.f},										// minor pentatonic
		Quantizer::Scale{0.f, 2.f, 4.f, 8.f, 10.f},										// wholetone
	};

	static constexpr std::array color = {
		Color{2, 2, 0},	  // off
		Palette::magenta, // chromatic
		Palette::blue,	  // major
		Palette::red,	  // minor
		Palette::cyan,	  // major pent
		Palette::yellow,  // minor pent
		Palette::pink,	  // wholetone
		Palette::green,	  // gate
	};

	static constexpr uint8_t max = color.size() - 1;
	static constexpr uint8_t min = 0u;
	uint8_t val = min;

public:
	void Inc(int32_t dir) {
		auto temp = val + dir;
		val = std::clamp<int32_t>(temp, min, max);
	}
	bool IsGate() {
		return val == max;
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
	bool Validate() {
		return val <= max;
	}
};

} // namespace Catalyst2::Channel