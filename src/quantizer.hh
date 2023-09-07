#pragma once

#include "scales.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

template<std::size_t range_octaves>
struct Quantizer {
	static constexpr float oct_size = (65536.f / range_octaves) + .5f;
	static constexpr float note_size = (65536.f / (range_octaves * 12)) + .5f;

	Quantizer()
	{
		load_scale(Scale{});
	};

	uint16_t process(const uint16_t input)
	{
		if (!scale.size())
			return input;

		auto value = static_cast<float>(input);
		uint8_t octave = 0;
		while (value >= oct_size) {
			value -= oct_size;
			octave += 1;
		}
		uint8_t note = 0;
		while (value >= note_size) {
			value -= note_size;
			note += 1;
		}

		value = note + (value / note_size);

		auto upper = *(std::upper_bound(scale.begin(), scale.end(), value) - 1);

		return (upper * note_size) + (octave * oct_size);
	}
	void load_scale(const Scale &scl)
	{
		scale = scl;
	}
	void enable(Scale &scl)
	{
		load_scale(scl);
	}
	void disable()
	{
		Scale empty;
		load_scale(empty);
	}

private:
	Scale scale{};
};

} // namespace Catalyst2