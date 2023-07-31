#pragma once
#include "conf/board_conf.hh"
#include "debug.hh"
#include "drivers/adc_builtin.hh"
// #include "drivers/analog_in_ext.hh"
#include "drivers/debounced_switch.hh"
#include "util/filter.hh"

namespace Catalyst2
{

class Controls {
	// ADCs (Pots and CV):
	// static inline std::array<uint16_t, NumAdcs> cv_adc_buffer;
	// mdrivlib::AdcDmaPeriph<Brain::CVAdcConf> cv_adcs{cv_adc_buffer,
	//                                                  Board::CVAdcChans};
	// std::array<Oversampler<256, uint16_t>, NumPots> pots;

public:
	Controls() = default;

	// Buttons/Switches:
	// Board::ModeSwitch mode_switch;
	using SwitchPos = mdrivlib::SwitchPos;

	// Trig Jacks
	// Board::HoldJack inf_jack;

	uint16_t read_pot(Board::AdcElement adcnum)
	{
		// return pots[adcnum].val();
	}

	SwitchPos read_mode_switch()
	{
		// return static_cast<SwitchPos>(time_switch.read());
		return {};
	}

	void start()
	{
		// pot_adcs.register_callback([this] {
		//   for (unsigned i = 0; auto &pot : pots)
		//     pot.add_val(pot_adc_buffer[i++]);
		// });
		// pot_adcs.start();
	}

	void update()
	{
		// MUX IO
		// Trig jacks
		// rev_jack.update();
		// inf_jack.update();
	}
};
} // namespace Catalyst2
