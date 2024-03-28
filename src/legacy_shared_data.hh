#pragma once
#include "channel.hh"
#include "drivers/flash_block.hh"
#include "drivers/flash_sectors.hh"
#include "legacy/v1_1/macro.hh"
#include "util/wear_level.hh"
#include "validate.hh"
#include <cstdint>

namespace Catalyst2::Legacy::V1_0
{

constexpr auto legacy_addr_0 = get_sector_addr(2);
constexpr auto legacy_addr_1 = get_sector_addr(3);
constexpr auto legacy_addr_2 = get_sector_addr(4);

constexpr uint32_t block_start_addr = legacy_addr_0;
constexpr uint32_t legacy_sector_size = 0x18000; // Not an actual sector! But that's what we told it

struct MacroSharedData : Macro::Data {
	Shared::Data shared{};
	bool validate() const {
		return true;
	}
	bool isMacroOk() const {
		return Macro::Data::validate();
	}
	bool isSharedOk() const {
		return shared.Validate();
	}
};

constexpr uint32_t block_size = sizeof(MacroSharedData); // 0x1eac

using SharedFlashBlock = WearLevel<mdrivlib::FlashBlock<MacroSharedData, block_start_addr, legacy_sector_size>>;

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
