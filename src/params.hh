#pragma once
#include "flags.hh"
#include "pathway.hh"
#include "quantizer.hh"
#include "scene.hh"
#include "sequencer.hh"

namespace Catalyst2
{

// Params holds all the modes, settings and parameters
// Params are set by UI, based on reading of the user input
struct Params {
	Flags flags;
	Banks banks;
	Pathway pathway;
	Sequencer seq;
	Quantizer quan{Model::scales};

	// coefficient of fading from scene to scene
	// f(x) = (1.f / (1.f - morph_step)) * x;
	float morph_step = 0.f;
	float slider_pos;
	float cv_offset = 0.f;
	float pos = 0.f;

	// TODO: clarify these and name them better, and add/remove as needed:
	enum class Mode : bool { Sequencer, Macro };
	Mode mode = Mode::Macro;
};

} // namespace Catalyst2
