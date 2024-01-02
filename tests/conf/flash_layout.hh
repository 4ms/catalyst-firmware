#define APP_FLASH_ADDR 0x08000000
#define APP_START_ADDR 0x08000000

#ifdef __cplusplus
#pragma once

// TODO: adjust these when we add
// 1) Bootloader
// 2) Storing data into flash

#include <cstdint>

//#include "drivers/flash_sectors.hh"

namespace Catalyst2
{

// constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0);	  // 32k Bootloader
constexpr inline uint32_t AppFlashAddr = get_sector_addr(0); // 208k = 128k+64k+16k app
// constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(6); // 256k to receive

constexpr inline uint32_t AppStartAddr = AppFlashAddr;
// constexpr inline uint32_t BootloaderStartAddr = BootloaderFlashAddr;

static constexpr uint32_t MacroSettingsFlashAddr = get_sector_addr(5); // == 0x0802'0000, Eventually will be sector 1
static constexpr uint32_t SeqSettingsFlashAddr = get_sector_addr(6);   // == 0x0804'0000, Eventually will be sector 2
static constexpr uint32_t SettingsSectorSize =
	SeqSettingsFlashAddr - MacroSettingsFlashAddr; // == 128k. Eventually will be 16k

static_assert(APP_FLASH_ADDR == AppFlashAddr);
static_assert(APP_START_ADDR == AppStartAddr);

} // namespace Catalyst2

#endif
