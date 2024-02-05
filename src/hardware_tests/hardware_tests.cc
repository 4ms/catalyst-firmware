#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/adc.hh"
#include "hardware_tests/buttons.hh"
#include "hardware_tests/dac.hh"
#include "hardware_tests/encoders.hh"
#include "hardware_tests/jacks.hh"
#include "hardware_tests/leds.hh"
#include "hardware_tests/util.hh"

namespace Catalyst2
{

static bool check_enter_hw_test() {
	Pin{Board::MuxSelectPins[0], Board::PinMode::Output}.off();
	Pin{Board::MuxSelectPins[1], Board::PinMode::Output}.off();
	Pin{Board::MuxSelectPins[2], Board::PinMode::Output}.off();

	Pin MuxRead0{Board::MuxInputChipPins[0], Board::PinMode::Input};
	Pin MuxRead1{Board::MuxInputChipPins[1], Board::PinMode::Input};

	return MuxRead0.read_raw() && MuxRead1.read_raw();
}

void run_hardware_test() {
	using namespace HWTests;

	if (!check_enter_hw_test())
		return;

	Controls controls;
	UtilIF::link_controls(controls);

	controls.Start();

	mdrivlib::Timekeeper controls_update_task{
		{
			.TIMx = TIM3,
			.period_ns = mdrivlib::TimekeeperConfig::Hz(500),
			.priority1 = 1,
			.priority2 = 0,
		},
		[&]() { controls.Update(); },
	};

	controls_update_task.start();

	Util::flash_mainbut_until_pressed();

	TestLEDs ledtester;
	ledtester.run_test();

	// TestButtons buttontester;
	// buttontester.run_test();

	auto dac_test = TestDac{controls};
	dac_test.run_test();

	auto adc_test = TestAdc{controls};
	adc_test.run_test();

	auto jack_test = TestJacks{controls};
	jack_test.run_test();

	auto enc_test = TestEncoders{controls};
	enc_test.run_test();

	uint32_t animtm = 0;
	uint32_t anim_step = 0;
	constexpr float Chans = Model::NumChans - 1.f;
	while (true) {
		if (HAL_GetTick() - animtm > 100) {
			animtm = HAL_GetTick();
			controls.SetButtonLed(anim_step % Model::NumChans, false);
			anim_step++;

			for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
				float offset = (i + anim_step) % Model::NumChans;
				controls.SetEncoderLed(i, Palette::yellow.blend(Palette::blue, offset / Chans));
			}

			controls.SetButtonLed(anim_step % Model::NumChans, true);
		};
	}

	controls_update_task.stop();
}

} // namespace Catalyst2
