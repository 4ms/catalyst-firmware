#pragma once
#include "controls.hh"
#include "flags.hh"

namespace Catalyst2
{
// Params holds all the modes, settings and parameters for the looping delay
// Params are set by controls (knobs, jacks, buttons, etc)
struct Params {
	Controls &controls;
	Flags &flags;

	Params(Controls &controls, Flags &flags)
		: controls{controls}
		, flags{flags}
	{}

	void start()
	{
		controls.start();
	}

	void update()
	{
		controls.update();
	}

private:
	struct PotState {
		int16_t cur_val = 0;
		int16_t prev_val = 0;		  // old_i_smoothed_potadc
		int16_t track_moving_ctr = 0; // track_moving_pot
		int16_t delta = 0;			  // pot_delta
		bool moved = false;			  // flag_pot_changed
	} slider_state;
};

} // namespace Catalyst2
