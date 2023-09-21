#pragma once
#include "pathway.hh"
#include "quantizer.hh"
#include "scene.hh"
#include "sequencer.hh"
#include <optional>

namespace Catalyst2
{

// Params holds all the modes, settings and parameters
// Params are set by UI, based on reading of the user input
struct Params {
	uint8_t current_bank = 0;
	Banks banks{current_bank};
	Pathway pathway{current_bank};
	Sequencer seq;
	Quantizer<static_cast<std::size_t>(Model::output_octave_range)> quantizer;
	int8_t current_scale = 0;

	std::optional<uint8_t> override_output;

	// coefficient of fading from scene to scene
	// f(x) = (1.f / (1.f - morph_step)) * x;
	float morph_step = 0.f;
	float pos = 0.f;

	// TODO: clarify these and name them better, and add/remove as needed:
	enum class Mode : bool { Sequencer, Macro };
	Mode mode = Mode::Macro;
};

} // namespace Catalyst2
