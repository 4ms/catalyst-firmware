#pragma once
#include "conf/palette.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/LEDTester.hh"

namespace Catalyst2::HWTests
{

struct TestLEDs : ILEDTester {
	TestLEDs()
		: ILEDTester{Model::NumChans + 4}
	{}

	void set_led(int led_id, bool turn_on) override
	{
		if (led_id < (int)Model::NumChans) {
			UtilIF::controls->SetButtonLed(led_id, turn_on);
		} else {
			// all encoder leds same color
			Color color = turn_on == false ? Palette::black :
						  led_id == 8	   ? Palette::red :
						  led_id == 9	   ? Palette::green :
						  led_id == 10	   ? Palette::blue :
											 Palette::white;

			for (unsigned i = 0; i < Model::NumChans; i++)
				UtilIF::controls->SetEncoderLed(i, color);
		}
	}

	void pause_between_steps() override
	{
		HAL_Delay(300);
	}
};
} // namespace Catalyst2::HWTests
