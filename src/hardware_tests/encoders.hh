#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"

namespace Catalyst2::HWTests
{

struct TestEncoders {
	Controls &controls;

	TestEncoders(Controls &controls)
		: controls{controls}
	{}

	void run_test()
	{
		std::array<uint8_t, 8> rotvals{0};
		while (true) {
			for (unsigned i = 0; auto &r : rotvals) {
				r += controls.encoders[i].read() * 8;
				controls.set_encoder_led(i, Palette::orange.blend(Palette::blue, r));
				i++;
			}

			if (Util::main_button_pressed())
				break;

			HAL_Delay(10);
		}
		while (!Util::main_button_pressed())
			;
	}
};

} // namespace Catalyst2::HWTests
