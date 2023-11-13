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
		: controls{controls} {
	}

	void run_test() {
		while (true) {
			controls.ForEachEncoderInc([this](uint8_t encoder, int32_t dir) {
				controls.SetEncoderLed(encoder, dir > 0 ? Palette::blue : Palette::red);
			});

			HAL_Delay(10);

			if (Util::main_button_pressed())
				break;
		}

		while (Util::main_button_pressed())
			;
	}
};

} // namespace Catalyst2::HWTests
