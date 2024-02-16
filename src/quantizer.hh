#pragma once

#include "channel.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Quantizer
{

struct Scale {
	static constexpr auto MaxScaleNotes = 22;

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

		float octaves_exact = input / Cv::octave;
		auto octave = static_cast<uint8_t>(octaves_exact);
		input -= octave * Cv::octave;

		float note = (float)input / (float)Cv::note;

		// lower bound is first element that is >= note
		auto lb = std::lower_bound(scale.begin(), scale.end(), note);

		float upper = *lb;
		float lower = lb == scale.begin() ? 0.f : *std::next(lb, -1);
		float closest = (std::abs(note - lower) <= std::abs(upper - note)) ? lower : upper;

		return (closest * Cv::note) + (octave * Cv::octave);
	}
	void Load(const Scale &scl) {
		scale = scl;
	}
};

} // namespace Catalyst2::Quantizer
