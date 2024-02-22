#pragma once
#include "conf/model.hh"

namespace Catalyst2::BuildOptions
{

inline constexpr auto seq_gate_overrides_prev_step = true;
inline constexpr auto default_mode = Model::Mode::Sequencer;
inline constexpr bool ManualColorMode = false;

#define SKIP_STARTUP_ANIMATION false

//////////////////////////////////////////////////////////////////////

#if SKIP_STARTUP_ANIMATION
#warning "Remember to turn the startup animation on!"
#endif

inline constexpr auto skip_startup_animation = SKIP_STARTUP_ANIMATION;
static_assert(skip_startup_animation == SKIP_STARTUP_ANIMATION);

} // namespace Catalyst2::BuildOptions
