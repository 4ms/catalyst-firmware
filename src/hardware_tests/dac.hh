#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"
#include "outputs.hh"

namespace Catalyst2::HWTests
{

struct TestDac {
	Controls &controls;
	Outputs outputs;
	std::array<uint8_t, 8> rotvals{0};
	Model::OutputBuffer outs;

	TestDac(Controls &controls)
		: controls{controls}
	{}

	void run_test()
	{
		while (true) {

			for (unsigned i = 0; auto &r : rotvals) {
				r += controls.encoders[i].read();
				controls.set_encoder_led(i, Palette::red.blend(Palette::blue, r));

				outs[i] = r * 8;
				outputs.write(outs);

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
