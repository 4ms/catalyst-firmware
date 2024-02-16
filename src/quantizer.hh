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
	Channel::Cv::type Process(Channel::Cv::type input) {
		if (!scale.size()) {
			return input;
		}
		using namespace Channel;
		uint8_t octave = 0;
		while (input >= Cv::octave) {
			input -= Cv::octave;
			octave += 1;
		}
		uint8_t note = 0;
		while (input >= Cv::note) {
			input -= Cv::note;
			note += 1;
		}

		float value = note + (static_cast<float>(input) / Cv::note);
		auto lb = std::lower_bound(scale.begin(), scale.end(), value);

		if (lb == scale.end()) {
			if (std::abs(12.f - value) < std::abs(value - *(lb - 1))) {
				lb = scale.begin();
				octave += 1;
			}
		}
		float closest = *lb;

		if (lb != scale.begin()) {
			auto ub = lb - 1;
			if (std::abs(value - *ub) < std::abs(value - closest)) {
				closest = *ub;
			}
		}
		return (closest * Cv::note) + (octave * Cv::octave);
	}
	void Load(const Scale &scl) {
		scale = scl;
	}
};

} // namespace Catalyst2::Quantizer
