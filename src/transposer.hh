#pragma once

#include "channel.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{
struct Transposer {
	using type = int8_t;
	static constexpr type min = 0;
	static constexpr type max = 12;

	static Model::Output::type Process(Model::Output::type input, type val) {
		const auto t = static_cast<int32_t>(input + (val * Channel::note));
		return std::clamp<int32_t>(t, Channel::min, Channel::max);
	}
};
} // namespace Catalyst2