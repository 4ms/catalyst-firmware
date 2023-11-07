#pragma once

#include "channelvalue.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{
struct Transposer {
	using type = int8_t;
	static constexpr type min = 0;
	static constexpr type max = 12;

	static ChannelValue::type Process(ChannelValue::type input, type val) {
		const auto t = static_cast<int32_t>(input + (val * ChannelValue::note));
		return std::clamp<int32_t>(t, ChannelValue::Min, ChannelValue::Max);
	}
};
} // namespace Catalyst2