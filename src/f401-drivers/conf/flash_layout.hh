#pragma once

// TODO: adjust these when we add
// 1) Bootloader
// 2) Storing data into flash

#define APP_FLASH_ADDR 0x08000000
#define APP_START_ADDR 0x08000000

#ifdef __cplusplus
#include <cstdint>

#include "drivers/flash_sectors.hh"

// constexpr inline uint32_t BootloaderFlashAddr = get_sector_addr(0);	  // 32k Bootloader
// constexpr inline uint32_t SettingsFlashAddr = get_sector_addr(2);	  // 16k Settings
constexpr inline uint32_t AppFlashAddr = get_sector_addr(0); // 208k = 128k+64k+16k app
// constexpr inline uint32_t BootloaderReceiveAddr = get_sector_addr(6); // 256k to receive

constexpr inline uint32_t AppStartAddr = AppFlashAddr;
// constexpr inline uint32_t BootloaderStartAddr = BootloaderFlashAddr;

static_assert(APP_FLASH_ADDR == AppFlashAddr);
static_assert(APP_START_ADDR == AppStartAddr);

#endif
