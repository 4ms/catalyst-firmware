#pragma once

#include <cstdint>
#include <type_traits>

namespace Catalyst2
{

static_assert(sizeof(bool) == 1);

using underlying_bool_type = uint8_t;

inline bool validateBool(underlying_bool_type in) {
	return in == static_cast<underlying_bool_type>(true) || in == static_cast<underlying_bool_type>(false);
}

} // namespace Catalyst2
