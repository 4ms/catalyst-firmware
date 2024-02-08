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
// using DataWithGlobalNonVolatileState = std::conditional<macro_is_smaller, Macro::Data, Sequencer::Data>::type;

inline constexpr auto NumSeqSlots = 8u;

struct SequencerData {
	Shared::Data shared;
	unsigned startup_slot = 0;
	std::array<Sequencer::Data, NumSeqSlots> slots;

	bool validate() const {
		auto ret = true;

		ret &= shared.Validate();

		for (auto &slot : slots)
			ret &= slot.validate();

		ret &= (startup_slot < NumSeqSlots);

		return ret;
	}
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
	Sequencer::Interface sequencer{data.sequencer.slots[0], shared};
	Macro::Interface macro{data.macro, shared};
};

inline constexpr auto params_size = sizeof(Params);

} // namespace Catalyst2
