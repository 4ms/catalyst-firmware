/* These are used in the linker script via the preprocessor */
#define BOOTLOADER_FLASH_START 0x08000000
#define BOOTLOADER_FLASH_SIZE 0x00008000
#define APP_FLASH_START 0x08040000
#define APP_FLASH_SIZE 0x00020000

#ifdef __cplusplus
#pragma once

#include <cstdint>

#include "drivers/flash_sectors.hh"

namespace Catalyst2
{

constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0); // sectors 0,1: 32k Bootloader
constexpr inline uint32_t AlsoBootloader = get_sector_addr(1);

constexpr inline uint32_t Empty2 = get_sector_addr(2);
constexpr inline uint32_t SharedSettingsFlashAddr = get_sector_addr(3);

constexpr inline uint32_t MacroSettingsFlashAddr = get_sector_addr(4); // sector 4: 64k for macro settings

constexpr inline uint32_t SeqSettingsFlashAddr = get_sector_addr(5);  // sector 5: 128k for sequencer settings
constexpr inline uint32_t AppFlashAddr = get_sector_addr(6);		  // sector 6: 128k for app
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(7); // sector 7: 128k to receive new firmware

constexpr inline uint32_t MacroSettingsSectorSize = SeqSettingsFlashAddr - MacroSettingsFlashAddr;

// What we want: (128k)
// constexpr inline uint32_t SeqSettingsSectorSize = AppFlashAddr - SeqSettingsFlashAddr;

// What v1.0 has: 96k
constexpr inline uint32_t SeqSettingsSectorSize = get_sector_addr(5) - get_sector_addr(2);

constexpr inline uint32_t SharedSettingsSectorSize = 16 * 1024;

static_assert(BootloaderFlashAddr == BOOTLOADER_FLASH_START);
static_assert(AppFlashAddr == APP_FLASH_START);

} // namespace Catalyst2

#endif
