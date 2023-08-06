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
	Part part;
	// Sequencer seq{64};

	// coefficient of fading from scene to scene
	float morph_step = 1.f;
	float cv_offset = 0.f;

	// TODO: clarify these and name them better, and add/remove as needed:
	enum class Mode : bool { Sequencer, Macro };
	Mode mode;

	enum class SeqMode { Multi, Single };
	SeqMode seqmode = SeqMode::Multi;

	enum class MacroMode { Classic, Pathway, OutofOrderSequence };
	MacroMode macromode = MacroMode::Classic;
};

} // namespace Catalyst2
