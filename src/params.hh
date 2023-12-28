#pragma once

#include "clock.hh"
#include "macro.hh"
#include "pathway.hh"
#include "quantizer.hh"
#include "recorder.hh"
#include "sequencer.hh"
#include "shared.hh"
#include "util/countzip.hh"
#include <array>
#include <optional>

namespace Catalyst2
{

// TODO: this stuff isn't really being used but it seems possible to automate these structs... not sure how yet
inline constexpr auto seqsize = sizeof(Sequencer::Data);
inline constexpr auto macrosize = sizeof(Macro::Data);
inline constexpr auto macro_is_smaller = macrosize < seqsize;
using DataWithGlobalNonVolatileState = std::conditional<macro_is_smaller, Macro::Data, Sequencer::Data>::type;

struct SequencerData : public Sequencer::Data {
	Shared::Data shared;
};

struct MacroData : public Macro::Data {
	// Shared::Data shared;
};

struct Data {
	MacroData macro;
	SequencerData sequencer;
};

struct Params {
	Data data;
	Shared::Interface shared{data.sequencer.shared};
	Catalyst2::Sequencer::Interface sequencer{data.sequencer, shared};
	Catalyst2::Macro::Interface macro{data.macro, shared};
};

inline constexpr auto params_size = sizeof(Params);

} // namespace Catalyst2
