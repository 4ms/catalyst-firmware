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
#include <type_traits>

namespace Catalyst2
{

// there is non volatile data that is shared between sequencer and macro mode
// the data is stored internally with whichever mode's non volatile data structure is smaller

inline constexpr auto macro_is_smaller = sizeof(Macro::Data) < sizeof(Sequencer::Data);

struct Data {
	struct Sequencer : Catalyst2::Sequencer::Data {
		std::conditional_t<!macro_is_smaller, Shared::Data, Shared::Dummy> shared{};

		bool validate() const {
			auto ret = true;
			ret &= shared.Validate();
			ret &= Sequencer::Data::validate();
			return ret;
		}
	};

	struct Macro : Catalyst2::Macro::Data {
		std::conditional_t<macro_is_smaller, Shared::Data, Shared::Dummy> shared{};

		bool validate() const {
			auto ret = true;
			ret &= shared.Validate();
			ret &= Macro::Data::validate();
			return ret;
		}
	};

	Macro macro{};
	Sequencer sequencer{};

private:
	friend struct Params;
	auto &shared() {
		if constexpr (macro_is_smaller) {
			return macro.shared;
		} else {
			return sequencer.shared;
		}
	}
};

inline constexpr auto data_size = sizeof(Data);

struct Params {
	Data data{};
	Shared::Interface shared{data.shared()};
	Sequencer::Interface sequencer{data.sequencer, shared};
	Macro::Interface macro{data.macro, shared};
};

inline constexpr auto params_size = sizeof(Params);

} // namespace Catalyst2
