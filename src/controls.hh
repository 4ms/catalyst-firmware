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
	static inline std::array<uint16_t, Board::NumAdcs> adc_buffer;
	mdrivlib::AdcDmaPeriph<Board::AdcConf> adc_dma{adc_buffer, Board::AdcChans};
	std::array<Oversampler<256, uint16_t>, Board::NumAdcs> analog;

public:
	Controls() = default;

	// Buttons/Switches:
	// Board::ModeSwitch mode_switch;

	// Trig Jacks
	// Board::HoldJack inf_jack;

	uint16_t read_pot(Board::AdcElement adcnum)
	{
		auto adc_chan_num = std::to_underlying(adcnum);
		return analog[adc_chan_num].val();
	}

	enum class ModeSwitch { Sequence, Macro };

	ModeSwitch read_mode_switch()
	{
		// return static_cast<SwitchPos>(time_switch.read());
		return {};
	}

	void start()
	{
		adc_dma.register_callback([this] {
			for (unsigned i = 0; auto &a : analog)
				a.add_val(adc_buffer[i++]);
		});
		adc_dma.start();
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
