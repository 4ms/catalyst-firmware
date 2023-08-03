#pragma once
#include "util/debouncer.hh"
// TODO: consider putting this in mdrivlib

namespace Catalyst2
{

struct MuxedButton : Toggler {
	uint8_t bit;

	MuxedButton(uint8_t bit_num)
		: bit{bit_num}
	{}

	void update(uint32_t raw_mux_read)
	{
		register_state(raw_mux_read & (1 << bit));
	}
};

} // namespace Catalyst2
