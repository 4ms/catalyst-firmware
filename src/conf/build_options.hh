#pragma once

namespace Catalyst2::BuildOptions
{

inline constexpr auto seq_gate_overrides_prev_step = true;

#define SKIP_STARTUP_ANIMATION true

//////////////////////////////////////////////////////////////////////

#if SKIP_STARTUP_ANIMATION
#warning "Remember to turn the startup animation on!"
#endif

inline constexpr auto skip_startup_animation = SKIP_STARTUP_ANIMATION;
static_assert(skip_startup_animation == SKIP_STARTUP_ANIMATION);

} // namespace Catalyst2::BuildOptions
