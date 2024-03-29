#pragma once
#include "drivers/flash_block.hh"
#include "drivers/flash_sectors.hh"
#include "legacy/v1_0/shared.hh"
#include <cstdint>

namespace Catalyst2::Legacy::V1_0
{

constexpr auto legacy_addr_0 = get_sector_addr(2);
constexpr auto legacy_addr_1 = get_sector_addr(3);
constexpr auto legacy_addr_2 = get_sector_addr(4);

constexpr uint32_t OffsetToSettingData = 0x1E8A;
constexpr uint32_t SettingsDataSize = sizeof(Shared::Data);
static_assert(SettingsDataSize == 0x22);
constexpr uint32_t Padding = 0x4; // Added by FlashBlock (extra padding bug)

constexpr uint32_t FirstSlot = Legacy::V1_0::legacy_addr_0 + Legacy::V1_0::OffsetToSettingData; // want 9e8a
constexpr uint32_t SecondSlot = FirstSlot + OffsetToSettingData + SettingsDataSize + Padding;	// want bd3a

inline bool eraseFlashSectors() {
	if (!mdrivlib::InternalFlash::erase_sector(legacy_addr_0)) {
		return false;
	}
	if (!mdrivlib::InternalFlash::erase_sector(legacy_addr_1)) {
		return false;
	}
	if (!mdrivlib::InternalFlash::erase_sector(legacy_addr_2)) {
		return false;
	}
	return true;
}

} // namespace Catalyst2::Legacy::V1_0
