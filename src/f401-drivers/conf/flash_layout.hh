/* These are used in the linker script via the preprocessor */
#define BOOTLOADER_FLASH_START 0x08000000
#define BOOTLOADER_FLASH_SIZE 0x00008000
#define APP_FLASH_START 0x08010000
#define APP_FLASH_SIZE 0x00030000

#ifdef __cplusplus
#pragma once

#include <cstdint>

#include "drivers/flash_sectors.hh"

namespace Catalyst2
{

constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0); // sectors 0,1: 32k Bootloader
constexpr inline uint32_t MacroSettingsFlashAddr = get_sector_addr(2);
constexpr inline uint32_t SeqSettingsFlashAddr = get_sector_addr(3);
constexpr inline uint32_t AppFlashAddr = get_sector_addr(4);		  // sectors 4,5: 192k app = 64k + 128k
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(6); // sectors 6,7: 256k to receive new firmware
constexpr inline uint32_t SettingsSectorSize = SeqSettingsFlashAddr - MacroSettingsFlashAddr;

static_assert(BootloaderFlashAddr == BOOTLOADER_FLASH_START);
static_assert(AppFlashAddr == APP_FLASH_START);

} // namespace Catalyst2

#endif
