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
	constexpr Scale(float octave, T... ts)
		: octave{fromFloat(octave)}
		, scl{static_cast<Channel::Cv::type>(fromFloat(ts))...}
		, size_(sizeof...(T)) {
	}
	Scale(FixedVector<Channel::Cv::type, MaxScaleNotes> &notes) {
		if (notes.size() > 2) {
			std::sort(notes.begin(), notes.end());
			notes.erase(std::unique(notes.begin(), notes.end()), notes.end());
			const auto last_note = notes[notes.size() - 1];
			const auto octave_size = last_note - notes[0];
			notes.erase(notes.size() - 1);
			for (auto &note : notes) {
				if (note / octave_size != notes[0] / octave_size) {
					const auto middle = &note;
					std::for_each(notes.begin(), middle, [octave_size](auto &i) { i += octave_size; });
					std::rotate(notes.begin(), middle, notes.end());
					break;
				}
			}

			const auto cur_octave = notes[0] / octave_size;
			const auto offset = cur_octave * octave_size;

			for (auto i = 0u; i < notes.size(); i++) {
				scl[i] = notes[i] - offset;
			}
			size_ = notes.size();
			octave = octave_size;
		}
	}
	constexpr const Channel::Cv::type &operator[](const std::size_t idx) const {
		return scl[idx];
	}
	constexpr Channel::Cv::type &operator[](const std::size_t idx) {
		return scl[idx];
	}
	constexpr const Channel::Cv::type &back() const {
		return scl[size_ - 1];
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
	constexpr bool Validate() const {
		if (size_ > MaxScaleNotes) {
			return false;
		}
		if (size_ == 0) {
			return true;
		}
		if (octave >= back() / 2 || octave < -(back() / 2)) {
			return false;
		}
		for (auto &n : *this) {
			if (n > Channel::Cv::max) {
				return false;
			}
		}
		return true;
	}

	Channel::Cv::type octave = 0;

private:
	constexpr Channel::Cv::type fromFloat(float in) {
		return Channel::Cv::note * in;
	}
	std::array<Channel::Cv::type, MaxScaleNotes> scl;
	std::size_t size_ = 0;
};

using CustomScales = std::array<Scale, Model::num_custom_scales>;

inline constexpr std::array scale = {
	Scale{0.f},																   // none
	Scale{12.f, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f}, // chromatic
	Scale{12.f, 0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f},						   // major
	Scale{12.f, 0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 10.f},						   // minor
	Scale{12.f, 0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 11.f},						   // harmonic minor
	Scale{12.f, 0.f, 2.f, 4.f, 7.f, 9.f},									   // major pentatonic
	Scale{12.f, 0.f, 3.f, 5.f, 7.f, 10.f},									   // minor pentatonic
	Scale{12.f, 0.f, 2.f, 4.f, 6.f, 8.f, 10.f},								   // wholetone
	Scale{12.f, 0.f, 2.f, 4.f, 6.f, 7.f, 9.f, 10.f},						   // acoustic/lydian dom.
	Scale{12.f, 0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 10.f, 11.f},					   // Beebop
	Scale{12.f, 0.f, 1.f, 4.f, 6.f, 8.f, 10.f, 11.f},						   // enigmatic
	Scale{12.f, 0.f, 2.5f, 3.f, 4.f, 5.f, 7.f},								   // vietnamese
	Scale{12.f, 0.f, 3.f, 5.f, 7.f, 10.f},									   // Yo scale

	// 16-TET
	Scale{12.f, 0.f, 0.75f, 1.5f, 2.25f, 3.f, 3.75f, 4.5f, 5.25f, 6.f, 6.75f, 7.5f, 8.25f, 9.f, 9.75f, 10.5f, 11.25f},

	// 21-TET
	Scale{12.f,
		  0.f,
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
		  11.4285714285714f},
};

// Quantize and Channel::Cv assume 12-note octaves, and Channel::Mode:Scales must also be octave-based

inline Channel::Cv::type Process(const Scale &scale, Channel::Cv::type input) {
	if (scale.size() < 1) {
		return input;
	}
	using namespace Channel;

	const auto cur_octave = input / scale.octave;
	input -= cur_octave * scale.octave;

	const auto note = input;

	// lower bound is first element that is >= note
	const auto lb = std::lower_bound(scale.begin(), scale.end(), note);
	const auto upper = lb == scale.end() ? scale.octave : *lb;
	const auto lower = lb == scale.begin() ? scale.back() - scale.octave : *std::next(lb, -1);

	const auto closest = (std::abs(note - lower) <= std::abs(upper - note)) ? lower : upper;
	if (closest < 0 && !cur_octave) {
		return scale[0];
	}

	return closest + (cur_octave * scale.octave);
}

} // namespace Catalyst2::Quantizer
