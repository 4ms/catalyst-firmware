#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

class ClockDivider {
	static constexpr std::array<uint8_t, 5> divideroptions = {1, 2, 4, 8, 24};
	uint8_t divide_by = 1;
	uint8_t counter = 0;

public:
	bool Step()
	{
		bool ret = false;

		if (!counter)
			ret = true;

		counter += 1;
		if (counter >= divideroptions[divide_by])
			counter = 0;

		return ret;
	}
	void Reset()
	{
		counter = 0;
	}
	uint8_t GetDiv()
	{
		return divide_by;
	}
	void SetDiv(uint8_t div)
	{
		divide_by = std::clamp<uint8_t>(div, 0, divideroptions.size() - 1);
	}

private:
};
} // namespace Catalyst2
