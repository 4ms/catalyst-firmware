#pragma once
#include "conf/board_conf.hh"
#include "conf/palette.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/ButtonChecker.hh"

namespace Catalyst2::HWTests
{

struct TestButtons : IButtonChecker {
	TestButtons()
		: IButtonChecker{15} {
		reset();
		set_allowable_noise(10);
		set_min_steady_state_time(10);
	}

	bool _read_button(uint8_t channel) override {
		if (channel < 8)
			return UtilIF::controls->button.scene[channel].is_high();
		if (channel == 8)
			return UtilIF::controls->button.play.is_high();
		if (channel == 9)
			return UtilIF::controls->button.fine.is_high();
		if (channel == 10)
			return UtilIF::controls->button.morph.is_high();
		if (channel == 11)
			return UtilIF::controls->button.shift.is_high();
		if (channel == 12)
			return UtilIF::controls->button.bank.is_high();
		if (channel == 13)
			return UtilIF::controls->button.add.is_high();
		if (channel == 14)
			return UtilIF::controls->sense.trig.is_high();
		return false;
	}

	void _set_error_indicator(uint8_t channel, ErrorType err) override {
		UtilIF::controls->SetEncoderLed(7, err == ErrorType::None ? Palette::off : Palette::red);
	}

	void _set_indicator(uint8_t indicator_num, bool newstate) override {
		if (indicator_num < 8)
			UtilIF::controls->SetButtonLed(indicator_num, newstate);
		else if (indicator_num == 8)
			UtilIF::controls->SetPlayLed(newstate);
		else if (indicator_num == 9)
			UtilIF::controls->SetEncoderLed(1, newstate ? Palette::green : Palette::off);
		else if (indicator_num == 10)
			UtilIF::controls->SetEncoderLed(2, newstate ? Palette::green : Palette::off);
		else if (indicator_num == 11)
			UtilIF::controls->SetEncoderLed(5, newstate ? Palette::green : Palette::off);
		else if (indicator_num == 12)
			UtilIF::controls->SetEncoderLed(6, newstate ? Palette::green : Palette::off);
		else if (indicator_num == 13)
			UtilIF::controls->SetEncoderLed(7, newstate ? Palette::green : Palette::off);
		else if (indicator_num == 14)
			UtilIF::controls->SetEncoderLed(0, newstate ? Palette::green : Palette::off);
	}

	void _check_max_one_pin_changed() override {
		//
	}
};
} // namespace Catalyst2::HWTests
