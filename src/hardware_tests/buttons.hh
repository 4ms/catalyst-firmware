#pragma once
#include "conf/board_conf.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/ButtonChecker.hh"

namespace Catalyst2::HWTests
{

struct TestButtons : IButtonChecker {
	TestButtons()
		: IButtonChecker{16}
	{
		reset();
		set_allowable_noise(10);
		set_min_steady_state_time(10);
	}

	bool _read_button(uint8_t channel) override
	{
		if (channel < 8)
			return UtilIF::controls->get_scene_button(channel).is_pressed();
		if (channel == 8)
			return UtilIF::controls->play_button.button.is_pressed();
		if (channel == 9)
			return UtilIF::controls->a_button.button.is_pressed();
		if (channel == 10)
			return UtilIF::controls->latch_button.button.is_pressed();
		if (channel == 11)
			return UtilIF::controls->b_button.button.is_pressed();
		if (channel == 12)
			return UtilIF::controls->alt_button.button.is_pressed();
		if (channel == 13)
			return UtilIF::controls->bank_button.button.is_pressed();
		if (channel == 14)
			return UtilIF::controls->mode_switch.button.is_pressed();
		if (channel == 15)
			return UtilIF::controls->trig_jack_sense.button.is_pressed();
		return false;
	}

	void _set_error_indicator(uint8_t channel, ErrorType err) override
	{
		UtilIF::controls->set_encoder_led(7, err == ErrorType::None ? Colors::off : Colors::red);
	}

	void _set_indicator(uint8_t indicator_num, bool newstate) override
	{
		if (indicator_num < 8)
			UtilIF::controls->set_button_led(indicator_num, newstate);
		else
			UtilIF::controls->set_encoder_led(indicator_num - 8, Colors::green);
	}

	void _check_max_one_pin_changed() override
	{
		//
	}
};
} // namespace Catalyst2::HWTests
