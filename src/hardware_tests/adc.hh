#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"

namespace Catalyst2::HWTests
{

struct TestAdc {
	Controls &controls;
	unsigned slider_left_led = 0;
	unsigned slider_right_led = 2;
	unsigned cv_left_led = 1;
	unsigned cv_right_led = 3;
	unsigned slider_map[4] = {0, 2, 5, 7};
	unsigned cv_map[4] = {1, 3, 4, 6};

	TestAdc(Controls &controls)
		: controls{controls}
	{}

	void run_test()
	{
		while (true) {
			controls.set_encoder_led(slider_map[slider_left_led], Palette::black);
			controls.set_encoder_led(slider_map[slider_right_led], Palette::black);
			float slider = 4.f - controls.read_slider() * 4 / 4096.f; // 0..3
			slider_left_led = (unsigned)slider;
			slider_right_led = (slider_left_led + 1) & 0b11;
			float slider_phase = slider - (float)slider_left_led;
			controls.set_encoder_led(slider_map[slider_left_led], Palette::white.blend(Palette::black, slider_phase));
			controls.set_encoder_led(slider_map[slider_right_led], Palette::black.blend(Palette::white, slider_phase));

			controls.set_encoder_led(cv_map[cv_left_led], Palette::black);
			controls.set_encoder_led(cv_map[cv_right_led], Palette::black);
			float cv = controls.read_cv() * 4 / 4096.f; // 0..3
			cv_left_led = (unsigned)cv;
			cv_right_led = (cv_left_led + 1) & 0b11;
			float cv_phase = cv - (float)cv_left_led;
			controls.set_encoder_led(cv_map[cv_left_led], Palette::pink.blend(Palette::black, cv_phase));
			controls.set_encoder_led(cv_map[cv_right_led], Palette::black.blend(Palette::pink, cv_phase));

			if (Util::main_button_pressed())
				break;

			HAL_Delay(10);
		}
		while (!Util::main_button_pressed())
			;
	}
};

} // namespace Catalyst2::HWTests
