#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2
{

class ChannelMode {
	uint8_t val = 0;

public:
	void Inc(int32_t dir)
	{
		auto temp = val + dir;
		val = std::clamp<int32_t>(temp, 0, Model::ChannelModeCount - 1);
	}

	bool IsGate()
	{
		return val == Model::ChannelModeCount - 1;
	}

	bool IsQuantized()
	{
		return !IsGate();
	}

	QuantizerScale GetScale()
	{
		return Model::Scale[IsGate() ? 0 : val];
	}

	Color GetColor()
	{
		return Palette::ChannelMode[val];
	}
};

} // namespace Catalyst2