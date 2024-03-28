#pragma once

#include "channel.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Legacy::V1_0
{
struct Transposer {
	using type = int16_t;
	static constexpr auto num_octaves = 5;
	static constexpr auto inc_step = Channel::Cv::inc_step;
	static constexpr auto inc_step_fine = Channel::Cv::inc_step_fine;

	static constexpr type max = Channel::Cv::notes_in_octave * num_octaves * inc_step;
	static constexpr type min = -max;
	static constexpr type def = 0;

	static Channel::Cv::type Process(Channel::Cv::type input, type val) {
		using namespace Channel;
		const auto t = static_cast<int32_t>(input + val);
		return std::clamp<int32_t>(t, Cv::min, Cv::max);
	}
};
} // namespace Catalyst2::Legacy::V1_0
