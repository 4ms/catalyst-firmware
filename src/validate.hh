#pragma once

#include <cstdint>
#include <type_traits>

namespace Catalyst2
{

static_assert(sizeof(bool) == 1);

using underlying_bool_type = uint8_t;

inline bool validateBool(uint8_t in) {
	return in == 0x00 || in == 0x01;
}

} // namespace Catalyst2
