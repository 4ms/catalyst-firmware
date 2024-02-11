#pragma once

#include "channel.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{
struct Transposer {
	using type = int8_t;
	static constexpr type max = 12 * 5;
	static constexpr type min = -max;
	static constexpr type def = 0;

	static Channel::Cv::type Process(Channel::Cv::type input, type val) {
		using namespace Channel;
		const auto t = static_cast<int32_t>(input + (val * Cv::note));
		return std::clamp<int32_t>(t, Cv::min, Cv::max);
	}
};
} // namespace Catalyst2
