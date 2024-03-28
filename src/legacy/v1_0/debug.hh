#pragma once

#ifdef TESTPROJECT
#define GCC_OPTIMIZE_OFF
#else

#include "conf/board_conf.hh"

namespace Catalyst2::Legacy::V1_0::Debug
{

using Pin0 = Board::DebugPin;
using Pins = std::tuple<Pin0>;

template<unsigned PinN>
using Pin = std::tuple_element_t<PinN, Pins>;

template<unsigned PinN = 0>
struct ScopedPin {
	ScopedPin() {
		Pin<PinN>::high();
	}
	~ScopedPin() {
		Pin<PinN>::low();
	}
};

static_assert(std::is_same_v<Pin<0>, Pin0>);
}; // namespace Catalyst2::Legacy::V1_0::Debug

#define GCC_OPTIMIZE_OFF __attribute__((optimize("-O0")))

#endif
