#pragma once

#include "util/wear_level.hh"
#include <cstdint>
#include <utility>

namespace Catalyst2::Legacy::V1_1__V1_2::Shared
{

inline constexpr uint32_t tag = 0xABCDDCBA;

struct Data {
	uint32_t SettingsVersionTag;
	uint8_t reserved[36];

	bool validate() const {
		return SettingsVersionTag == tag;
	}
};

static_assert(sizeof(Data) == 40);
} // namespace Catalyst2::Legacy::V1_1__V1_2::Shared
