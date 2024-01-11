#pragma once

#include "channel.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Quantizer
{

struct Scale {
	static constexpr auto MaxScaleNotes = 12;

	template<typename... T>
	constexpr Scale(T... ts)
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

class Interface {
	Scale scale{};

public:
	Model::Output::type Process(const Model::Output::type input) {
		if (!scale.size()) {
			return input;
		}
		auto value = static_cast<float>(input);
		uint8_t octave = 0;
		while (value >= Channel::octave) {
			value -= Channel::octave;
			octave += 1;
		}
		uint8_t note = 0;
		while (value >= Channel::note) {
			value -= Channel::note;
			note += 1;
		}

		value = note + (value / Channel::note);
		auto lb = std::lower_bound(scale.begin(), scale.end(), value);
		if (lb == scale.end()) {
			lb -= 1;
		}
		return (*lb * Channel::note) + (octave * Channel::octave);
	}
	void Load(const Scale &scl) {
		scale = scl;
	}
};

} // namespace Catalyst2::Quantizer