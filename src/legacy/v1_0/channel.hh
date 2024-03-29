#pragma once
#include <cstdint>

namespace Catalyst2::Legacy::V1_0::Channel
{

namespace Cv
{
using type = uint16_t;
inline constexpr auto notes_in_octave = 12u;
inline constexpr type inc_step = 25u;
inline constexpr type max = 4500u;

inline bool Validate(type val) {
	return val <= max;
}
}; // namespace Cv

namespace Gate
{
using internal_type = uint8_t;
using type = float;

inline constexpr internal_type max = 15u;

inline bool Validate(internal_type val) {
	return val <= max;
}
}; // namespace Gate

} // namespace Catalyst2::Legacy::V1_0::Channel
