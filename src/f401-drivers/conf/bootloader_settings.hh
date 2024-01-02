#pragma once
#include "conf/flash_layout.hh"
#include <cstdint>

namespace BootloaderConf
{
static constexpr uint32_t SampleRate = 48000;

constexpr inline uint32_t ReceiveSectorSize = 16 * 1024; // -g param

struct FskEncoding {
	uint32_t blank;
	uint32_t one;
	uint32_t zero;
};
static constexpr FskEncoding Encoding{16, 8, 4};
}; // namespace BootloaderConf
