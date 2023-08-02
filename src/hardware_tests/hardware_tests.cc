#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/adc.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/encoders.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/util.hh"
#include "outputs.hh"

namespace Catalyst2
{

void run_hardware_test()
{
	using namespace HWTests;

	Controls controls;
	UtilIF::link_controls(controls);

	controls.start();

	mdrivlib::Timekeeper encoder_led_update_task{
		{
			.TIMx = TIM2,
			.period_ns = mdrivlib::TimekeeperConfig::Hz(120),
			.priority1 = 2,
			.priority2 = 0,
		},
		[&]() { controls.write_to_encoder_leds(); },
	};

	mdrivlib::Timekeeper controls_update_task{
		{
			.TIMx = TIM3,
			.period_ns = mdrivlib::TimekeeperConfig::Hz(500),
			.priority1 = 1,
			.priority2 = 0,
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

	auto adc_test = TestAdc{controls};
	adc_test.run_test();

	auto enc_test = TestEncoders{controls};
	enc_test.run_test();

	// TODO:
	// DACTest;

	while (true)
		;

	controls_update_task.stop();
	encoder_led_update_task.stop();
}

} // namespace Catalyst2
