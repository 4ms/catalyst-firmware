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
		Quantizer::Scale{},																 // none
		Quantizer::Scale{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f}, // chromatic
		Quantizer::Scale{2.f, 4.f, 5.f, 7.f, 9.f, 11.f, 12.f},							 // major
		Quantizer::Scale{2.f, 3.f, 5.f, 7.f, 8.f, 10.f, 12.f},							 // minor
		Quantizer::Scale{2.f, 3.f, 5.f, 7.f, 8.f, 11.f, 12.f},							 // harmonic minor
		Quantizer::Scale{2.f, 4.f, 7.f, 9.f, 12.f},										 // major pentatonic
		Quantizer::Scale{3.f, 5.f, 7.f, 10.f, 12.f},									 // minor pentatonic
		Quantizer::Scale{2.f, 4.f, 6.f, 8.f, 10.f, 12.f},								 // wholetone
		Quantizer::Scale{2.f, 4.f, 6.f, 7.f, 9.f, 10.f, 12.f},							 // acoustic/lydian dom.
		Quantizer::Scale{2.f, 4.f, 5.f, 7.f, 9.f, 10.f, 11.f, 12.f},					 // Beebop
		Quantizer::Scale{1.f, 4.f, 6.f, 8.f, 10.f, 11.f, 12.f},							 // enigmatic
		Quantizer::Scale{2.5f, 3.f, 4.f, 5.f, 7.f, 12.f},								 // vietnamese
		Quantizer::Scale{3.f, 5.f, 7.f, 10.f, 12.f},									 // Yo scale

		// 15-TET
		// Quantizer::Scale{0.8f, 1.6f, 2.4f, 3.2f, 4.f, 4.8f, 5.6f, 6.4f, 7.2f, 8.f, 8.8f, 9.6f, 10.4f, 11.2f, 12.f},

		// 16-TET
		Quantizer::Scale{
			0.75f, 1.5f, 2.25f, 3.f, 3.75f, 4.5f, 5.25f, 6.f, 6.75f, 7.5f, 8.25f, 9.f, 9.75f, 10.5f, 11.25f, 12.f},

		// 21-TET
		Quantizer::Scale{0.571428571428571f,
						 1.14285714285714f,
						 1.71428571428571f,
						 2.28571428571429f,
						 2.85714285714286f,
						 3.42857142857143f,
						 4.f,
						 4.57142857142857f,
						 5.14285714285714f,
						 5.71428571428571f,
						 6.28571428571429f,
						 6.85714285714286f,
						 7.42857142857143f,
						 8.f,
						 8.57142857142857f,
						 9.14285714285714f,
						 9.71428571428571f,
						 10.2857142857143f,
						 10.8571428571429f,
						 11.4285714285714f,
						 12.f},
	};

	// notice no -1 after size.
	// final "scale" is gate mode
	static constexpr uint8_t max = Scale.size();
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
		return Palette::Scales::color[val];
	}
	bool Validate() const {
		return val <= max;
	}

	// Quantize and Channel::Cv assume 12-note octaves, and Channel::Mode:Scales must also be octave-based
	static_assert([]() {
		for (auto scale : Scale) {
			if (scale.size() == 0)
				continue;
			if (scale[scale.size() - 1] != 12.f)
				return false;
		}
		return true;
	}());
}; // namespace Catalyst2::Channel

} // namespace Catalyst2::Channel
