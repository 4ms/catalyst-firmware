#pragma once
#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "debug.hh"
#include "drivers/adc_builtin.hh"
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

	Board::TrigJack trig_jack;
	Board::ResetJack reset_jack;

	std::array<mdrivlib::RotaryEncoder, Model::NumChans> encoders{{
		{Board::Enc1A, Board::Enc1B, Board::EncStepSize},
		{Board::Enc2A, Board::Enc2B, Board::EncStepSize},
		{Board::Enc3A, Board::Enc3B, Board::EncStepSize},
		{Board::Enc4A, Board::Enc4B, Board::EncStepSize},
		{Board::Enc5A, Board::Enc5B, Board::EncStepSize},
		{Board::Enc6A, Board::Enc6B, Board::EncStepSize},
		{Board::Enc7A, Board::Enc7B, Board::EncStepSize},
		{Board::Enc8A, Board::Enc8B, Board::EncStepSize},
	}};

	uint16_t read_pot(Model::AdcElement adcnum)
	{
		auto adc_chan_num = std::to_underlying(adcnum);
		return analog[adc_chan_num].val();
	}

	Model::ModeSwitch read_mode_switch()
	{
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
	}

	void set_led(unsigned led, float value)
	{
		if (led >= Model::NumChans)
			return;
		// else put the value in some buffer which is
		//  written to the LED Driver chip later
	}

	void set_leds(const std::array<float, 8> &values)
	{}
};
} // namespace Catalyst2
