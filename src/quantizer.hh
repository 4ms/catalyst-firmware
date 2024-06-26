#pragma once

#include "channel.hh"
#include "util/fixed_vector.hh"
#include "util/zip.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Quantizer
{

struct Scale {
	static constexpr auto MaxScaleNotes = 22;

	constexpr Scale() = default;

	template<typename... T>
	constexpr Scale(float offset, T... ts)
		: offset{fromFloat(offset)}
		, scl{static_cast<Channel::Cv::type>(fromFloat(ts))...}
		, size_(sizeof...(T)) {
	}
	Scale(FixedVector<Channel::Cv::type, MaxScaleNotes> &notes) {
		std::sort(notes.begin(), notes.end());
		notes.erase(std::unique(notes.begin(), notes.end()), notes.end());
		const int first_note = notes[0];
		notes.erase(0);
		if (notes.size() > 1) {
			for (auto i = 0u; i < notes.size(); i++) {
				scl[i] = notes[i] - first_note;
			}
			size_ = notes.size();
			const auto last_note = scl[size_ - 1];
			offset = first_note % last_note;
			if (offset >= (last_note / 2)) {
				offset -= last_note;
			}
		}
	}
	constexpr const Channel::Cv::type &operator[](const std::size_t idx) const {
		return scl[idx];
	}
	constexpr Channel::Cv::type &operator[](const std::size_t idx) {
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
	constexpr auto begin() {
		return scl.begin();
	}
	constexpr auto end() {
		return begin() + size_;
	}

	int16_t offset = 0;

private:
	constexpr int16_t fromFloat(float in) {
		return Channel::Cv::note * in;
	}
	std::array<Channel::Cv::type, MaxScaleNotes> scl;
	std::size_t size_ = 0;
};

using CustomScales = std::array<Scale, Model::num_custom_scales>;

inline constexpr std::array scale = {
	Scale{0.f},																   // none
	Scale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f}, // chromatic
	Scale{0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f, 12.f},						   // major
	Scale{0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 10.f, 12.f},						   // minor
	Scale{0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 11.f, 12.f},						   // harmonic minor
	Scale{0.f, 2.f, 4.f, 7.f, 9.f, 12.f},									   // major pentatonic
	Scale{0.f, 3.f, 5.f, 7.f, 10.f, 12.f},									   // minor pentatonic
	Scale{0.f, 2.f, 4.f, 6.f, 8.f, 10.f, 12.f},								   // wholetone
	Scale{0.f, 2.f, 4.f, 6.f, 7.f, 9.f, 10.f, 12.f},						   // acoustic/lydian dom.
	Scale{0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 10.f, 11.f, 12.f},					   // Beebop
	Scale{0.f, 1.f, 4.f, 6.f, 8.f, 10.f, 11.f, 12.f},						   // enigmatic
	Scale{0.f, 2.5f, 3.f, 4.f, 5.f, 7.f, 12.f},								   // vietnamese
	Scale{0.f, 3.f, 5.f, 7.f, 10.f, 12.f},									   // Yo scale

	// 16-TET
	Scale{0.f, 0.75f, 1.5f, 2.25f, 3.f, 3.75f, 4.5f, 5.25f, 6.f, 6.75f, 7.5f, 8.25f, 9.f, 9.75f, 10.5f, 11.25f, 12.f},

	// 21-TET
	Scale{0.f,
		  0.571428571428571f,
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

// Quantize and Channel::Cv assume 12-note octaves, and Channel::Mode:Scales must also be octave-based

inline Channel::Cv::type Process(const Scale &scale, Channel::Cv::type input) {
	if (scale.size() < 1) {
		return input;
	}
	using namespace Channel;

	const auto scale_octave = scale[scale.size() - 1];
	const auto octave = static_cast<uint8_t>(input / static_cast<float>(scale_octave));
	input -= octave * scale_octave;

	const auto note = input;

	// lower bound is first element that is >= note
	const auto lb = std::lower_bound(scale.begin(), scale.end(), note);

	const float upper = *lb;
	const float lower = lb == scale.begin() ? 0.f : *std::next(lb, -1);
	const float closest = (std::abs(note - lower) <= std::abs(upper - note)) ? lower : upper;

	return (closest + (octave * scale_octave)) + scale.offset;
}

} // namespace Catalyst2::Quantizer
