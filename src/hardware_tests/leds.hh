#pragma once
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
		if (led_id < Model::NumChans) {
			UtilIF::controls->set_button_led(led_id, turn_on);
		} else {
			// all encoder leds same color
			Color color = led_id == 8  ? Colors::red :
						  led_id == 9  ? Colors::green :
						  led_id == 10 ? Colors::blue :
										 Colors::white;

			for (unsigned i = 0; i < Model::NumChans; i++)
				UtilIF::controls->set_encoder_led(i, color);
		}
	}

	void pause_between_steps() override
	{
		Util::pause_until_button_pressed();
		Util::pause_until_button_released();
	}
};
} // namespace Catalyst2::HWTests
