#pragma once
#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Scene {
	using ChannelValue_t = uint16_t;

	enum class ChannelType { CV, Gate };

	std::array<ChannelValue_t, Model::NumChans> chans;
	std::array<ChannelType, Model::NumChans> types;
};

using SceneBanks = std::array<Scene, Model::NumBanks>;

} // namespace Catalyst2
