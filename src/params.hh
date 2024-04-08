#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "macro.hh"
#include "pathway.hh"
#include "quantizer.hh"
#include "recorder.hh"
#include "sequencer.hh"
#include "shared.hh"
#include "util/countzip.hh"
#include <array>
#include <optional>
#include <type_traits>

namespace Catalyst2
{

// there is non volatile data that is shared between sequencer and macro mode
// the data is stored internally with whichever mode's non volatile data structure is smaller

struct Data {
	Shared::Data shared{};
	Macro::Data macro{};
	Sequencer::Data sequencer{};
};

inline constexpr auto macro_data_size = sizeof(Macro::Data); // 7820
inline constexpr auto seq_data_size = sizeof(Sequencer::Data);
inline constexpr auto data_size = sizeof(Data);

struct Params {
	Data data{};
	Shared::Interface shared{data.shared};
	Sequencer::Interface sequencer{data.sequencer, shared};
	Macro::Interface macro{data.macro, shared};
};

inline constexpr auto params_size = sizeof(Params);

} // namespace Catalyst2
