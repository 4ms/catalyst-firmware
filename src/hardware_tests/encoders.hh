#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"

namespace Catalyst2::HWTests
{

struct TestEncoders {
	Controls &controls;
	std::array<uint8_t, 8> rotvals{0};

	TestEncoders(Controls &controls)
		: controls{controls}
	{}

	void run_test()
	{
		while (true) {
			for (unsigned i = 0; auto &r : rotvals) {
				r += controls.GetEncoder(i) * 8;
				controls.SetEncoderLed(i, Palette::orange.blend(Palette::blue, r));
				i++;
			}

			HAL_Delay(10);

			if (Util::main_button_pressed())
				break;
		}

		while (Util::main_button_pressed())
			;
	}
};

} // namespace Catalyst2::HWTests
