#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct QuantizerScale {
	static constexpr auto MaxScaleNotes = 12;

	template<typename... T>
	constexpr QuantizerScale(T... ts)
		: scl{ts...}
		, size_(sizeof...(T)) {
	}
	constexpr const float &operator[](const std::size_t idx) const {
		return scl[idx];
	}
	constexpr std::size_t size() const {
		return size_;
	}
	constexpr auto begin() const {
		return scl.begin();
	}
	constexpr auto end() const {
		return begin() + size_;
	}

private:
	std::array<float, MaxScaleNotes> scl;
	std::size_t size_;
};

template<std::size_t range_octaves>
struct Quantizer {
	static constexpr float oct_size = (65536.f / range_octaves) + .5f;
	static constexpr float note_size = (65536.f / (range_octaves * 12)) + .5f;

	uint16_t Process(const uint16_t input) {
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

		value = *(std::upper_bound(scale.begin(), scale.end(), value) - 1);

		return (value * note_size) + (octave * oct_size);
	}
	void LoadScale(const QuantizerScale &scl) {
		scale = scl;
	}

private:
	QuantizerScale scale{};
};

} // namespace Catalyst2