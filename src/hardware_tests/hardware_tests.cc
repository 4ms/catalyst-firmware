#include "controls.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/util.hh"
#include "outputs.hh"

namespace Catalyst2
{
static Controls controls;

void run_hardware_test()
{
	using namespace HWTests;
	UtilIF::link_controls(controls);

	controls.start();

	mdrivlib::Timekeeper encoder_led_update_task{
		{
			.TIMx = TIM7,
			.period_ns = mdrivlib::TimekeeperConfig::Hz(120),
			.priority1 = 1,
			.priority2 = 1,
		},
		[&]() { controls.write_to_encoder_leds(); },
	};

	mdrivlib::Timekeeper controls_update_task{
		{
			.TIMx = TIM6,
			.period_ns = mdrivlib::TimekeeperConfig::Hz(1000),
			.priority1 = 1,
			.priority2 = 1,
		},
		[&]() { controls.update(); },
	};

	controls_update_task.start();
	encoder_led_update_task.start();

	Util::flash_mainbut_until_pressed();

	TestLEDs ledtester;
	ledtester.run_test();

	TestButtons buttontester;
	buttontester.run_test();

	while (true) {

		uint8_t slider = controls.read_slider() / 16U;
		controls.set_encoder_led(0, Colors::black.blend(Colors::white, slider));

		auto cv = controls.read_cv() / 4096.f;
		controls.set_encoder_led(1, Colors::black.blend(Colors::white, cv));

		Color color = controls.a_button.button.is_pressed();

		for (unsigned i = 0; i < Model::NumChans; i++) {
			bool pressed = controls.get_scene_button(i).is_pressed();
			controls.set_button_led(i, pressed);
		}

		HAL_Delay(10);
	}
}

} // namespace Catalyst2
