#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

class ClockDivider {
	static constexpr std::array<uint8_t, 12> divideroptions = {1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32};
	static_assert(divideroptions.size() <= 128,
				  "If you need more than 128 clock divider options, change the type from int8_t");
	uint8_t counter = 0;
	bool step = false;

public:
	using type = int8_t;
	void Update(type idx) {
		counter += 1;
		if (counter >= divideroptions[idx]) {
			step = true;
			counter = 0;
		}
	}
	bool Step() {
		bool ret = false;
		if (step) {
			ret = true;
			step = false;
		}
		return ret;
	}
	void Reset() {
		counter = 0;
		step = false;
	}
	static type IncDivIdx(type idx, int32_t inc) {
		int32_t t = idx;
		t += inc;
		return std::clamp<int32_t>(t, 0, divideroptions.size() - 1);
	}
	static uint8_t GetDivFromIdx(type idx) {
		return divideroptions[idx];
	}
};
} // namespace Catalyst2
