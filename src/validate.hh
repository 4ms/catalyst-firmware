#pragma once
#include <cstdint>

namespace Catalyst2
{

inline bool validateBool(uint8_t in) {
	return in == 0x00 || in == 0x01;
}

} // namespace Catalyst2
