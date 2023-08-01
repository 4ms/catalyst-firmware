#pragma once
#include "flags.hh"
#include "scene.hh"
#include "sequencer.hh"

namespace Catalyst2
{

// Params holds all the modes, settings and parameters
// Params are set by UI, based on reading of the user input

struct Params {
	Flags flags;
	SceneBanks scenes;
	Sequencer seq{64};

	unsigned cur_bank; //? do we need this? currently active bank?

	// coefficient of fading from scene to scene
	float morph_step = 1.f;

	// TODO: clarify these and name them better, and add/remove as needed:
	enum class Mode { Sequencer, Pathway, OutofOrderSequence };
	Mode mode;
};

} // namespace Catalyst2
