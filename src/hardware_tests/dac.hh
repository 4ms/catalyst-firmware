#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"
#include "outputs.hh"
#include "util/countzip.hh"

namespace Catalyst2::HWTests
{

struct TestDac {
	Controls &controls;
	Outputs outputs;
	std::array<uint8_t, Model::NumChans> rotvals{0};
	Model::OutputBuffer outs;

	TestDac(Controls &controls)
		: controls{controls}
	{}

	void run_test()
	{
		while (true) {

			for (auto [i, r] : countzip(rotvals)) {

				bool x = i == (controls.read_slider() >> 9);
				controls.set_button_led(i, x);
				r = 255 * x;
				controls.set_encoder_led(i, Palette::red.blend(Palette::blue, r));
				outs[i] = r << 8;
				outputs.write(outs);
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
