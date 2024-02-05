#pragma once
#include "conf/palette.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/LEDTester.hh"

namespace Catalyst2::HWTests
{

struct TestLEDs : ILEDTester {
	TestLEDs()
		: ILEDTester{Model::NumChans + 5} {
	}

	void set_led(int led_id, bool turn_on) override {
		if (led_id < (int)Model::NumChans) {
			UtilIF::controls->SetButtonLed(led_id, turn_on);
		} else if (led_id == Model::NumChans) {
			UtilIF::controls->SetPlayLed(turn_on);
		} else {
			// all encoder leds same color
			int first = Model::NumChans + 1;
			Color color = turn_on == false	  ? Palette::black :
						  led_id == first + 0 ? Palette::red :
						  led_id == first + 1 ? Palette::green :
						  led_id == first + 2 ? Palette::blue :
												Palette::full_white;

			for (unsigned i = 0; i < Model::NumChans; i++)
				UtilIF::controls->SetEncoderLed(i, color);
		}
	}

	void pause_between_steps() override {
		HAL_Delay(350);
	}
};
} // namespace Catalyst2::HWTests
