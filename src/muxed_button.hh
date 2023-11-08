#pragma once
#include "util/debouncer.hh"

namespace Catalyst2
{

struct MuxedButton : Toggler {
	unsigned time_high;

	MuxedButton(uint8_t bit_num)
		: bit{bit_num} {
	}

	void update(uint32_t raw_mux_read) {
		auto was_low = !this->is_high();
		register_state(raw_mux_read & (1 << bit));
		auto high = this->is_high();

		if (high && was_low) {
			time_high = 0;
		} else if (high) {
			time_high++;
		}
	}

private:
	const uint8_t bit;
};

} // namespace Catalyst2
