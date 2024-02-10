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

constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0);	   // sectors 0,1: 32k Bootloader
constexpr inline uint32_t MacroSettingsFlashAddr = get_sector_addr(2); // sector 2: 16k for macro settings
// sectors 3 (16k) and 4 (64k) are empty
constexpr inline uint32_t SeqSettingsFlashAddr = get_sector_addr(5);  // sector 5: 128k for sequencer settings
constexpr inline uint32_t AppFlashAddr = get_sector_addr(6);		  // sector 6: 128k for app
constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(7); // sector 7: 128k to receive new firmware
constexpr inline uint32_t SettingsSectorSize = SeqSettingsFlashAddr - MacroSettingsFlashAddr;

static_assert(BootloaderFlashAddr == BOOTLOADER_FLASH_START);
static_assert(AppFlashAddr == APP_FLASH_START);

} // namespace Catalyst2

#endif
