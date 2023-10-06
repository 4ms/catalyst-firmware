#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

class ClockDivider {
	static constexpr std::array<uint8_t, 12> divideroptions = {1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32};
	uint8_t div_idx = 0;
	uint8_t counter = 0;
	bool step = false;

public:
	void Update()
	{
		counter += 1;
		if (counter >= divideroptions[div_idx]) {
			step = true;
			counter = 0;
		}
	}
	bool Step()
	{
		bool ret = false;
		if (step) {
			ret = true;
			step = false;
		}
		return ret;
	}
	void Reset()
	{
		counter = 0;
		step = false;
	}
	void IncDiv(int32_t inc)
	{
		int32_t t = div_idx;
		t += inc;
		div_idx = std::clamp<int32_t>(t, 0, divideroptions.size() - 1);
	}
	uint8_t GetDiv()
	{
		return divideroptions[div_idx];
	}
};
} // namespace Catalyst2
