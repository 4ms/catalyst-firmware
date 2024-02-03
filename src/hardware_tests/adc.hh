#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/CodecCallbacks.hh"
#include "range.hh"

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

	SinOsc osc{0.5, Channel::from_volts(5.f) - Channel::from_volts(0.f), 0.f, SampleRate};
	Model::Output::Buffer outs;
	static constexpr auto SampleRate = 2000u;

	TestAdc(Controls &controls)
		: controls{controls} {
		// osc.init(0.5, 0.25f, Channel::from_volts(5.f), Channel::from_volts(0.f), 0.f, SampleRate);
		for (auto &out : outs)
			out = Channel::from_volts(0.f);
	}

	void run_test() {
		mdrivlib::Timekeeper dac_update_task{
			{
				.TIMx = TIM9,
				.period_ns = mdrivlib::TimekeeperConfig::Hz(SampleRate),
				.priority1 = 1,
				.priority2 = 1,
			},
			[this]() {
				outs[0] = osc.update() + Channel::from_volts(7.5f) - Channel::from_volts(0.f);
				controls.Write(outs);
			},
		};
		dac_update_task.start();

		while (true) {

			// Clear old slider position
			controls.SetEncoderLed(slider_map[slider_left_led], Palette::black);
			controls.SetEncoderLed(slider_map[slider_right_led], Palette::black);

			// Calc new slider position
			float slider = controls.ReadSlider() * 3 / 4096.f; // 0..3
			slider_left_led = (unsigned)slider;
			slider_right_led = (slider_left_led + 1) & 0b11;
			float slider_phase = slider - (float)slider_left_led;

			// Draw new slider position
			controls.SetEncoderLed(slider_map[slider_left_led],
								   Palette::full_white.blend(Palette::black, slider_phase));
			controls.SetEncoderLed(slider_map[slider_right_led],
								   Palette::black.blend(Palette::full_white, slider_phase));

			// Clear old CV position
			controls.SetEncoderLed(cv_map[cv_left_led], Palette::black);
			controls.SetEncoderLed(cv_map[cv_right_led], Palette::black);

			// Calc new CV position
			float cv = controls.ReadCv() * 3 / 4096.f; // 0..3
			cv_left_led = (unsigned)cv;
			cv_right_led = (cv_left_led + 1) & 0b11;
			float cv_phase = cv - (float)cv_left_led;

			// Draw new CV position
			controls.SetEncoderLed(cv_map[cv_left_led], Palette::pink.blend(Palette::black, cv_phase));
			controls.SetEncoderLed(cv_map[cv_right_led], Palette::black.blend(Palette::pink, cv_phase));

			if (Util::main_button_pressed())
				break;

			HAL_Delay(10);
		}

		dac_update_task.stop();

		while (Util::main_button_pressed())
			;
	}
};

} // namespace Catalyst2::HWTests
