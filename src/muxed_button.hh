#pragma once
#include "util/debouncer.hh"
// TODO: consider putting this in mdrivlib

namespace Catalyst2
{
// TODO: Is this a good way of assigning buttons to the raw bits from the Muxes?
// Replace if needed...
struct MuxedButton {
	Toggler button;
	uint8_t bit;

	MuxedButton(uint8_t bit_num)
		: bit{bit_num}
	{}

	void update(uint32_t raw_mux_read)
	{
		button.set_state(raw_mux_read & (1 << bit));
	}
};

} // namespace Catalyst2
