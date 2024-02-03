#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/CodecCallbacks.hh"
#include "outputs.hh"
#include "util/countzip.hh"

namespace Catalyst2::HWTests
{

struct TestDac {
	Controls &controls;
	Model::Output::Buffer outs;
	std::array<SkewedTriOsc, Model::NumChans> oscs;
	static constexpr auto SampleRate = 10000u;

	TestDac(Controls &controls)
		: controls{controls} {

		for (auto [i, osc] : enumerate(oscs)) {
			osc.init(20 + i * 20, 0.3f, 65535.f, 0.f, 0.f, SampleRate);
		}
	}

	void run_test() {
		for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
			controls.SetButtonLed(i, false);
			controls.SetEncoderLed(i, Palette::red.blend(Palette::blue, (float)i / ((float)Model::NumChans - 1.f)));
		}

		mdrivlib::Timekeeper dac_update_task{
			{
				.TIMx = TIM9,
				.period_ns = mdrivlib::TimekeeperConfig::Hz(SampleRate),
				.priority1 = 1,
				.priority2 = 1,
			},
			[this]() {
				for (auto [i, osc] : enumerate(oscs)) {
					outs[i] = osc.update();
				}
				controls.Write(outs);
			},
		};
		dac_update_task.start();

		Util::flash_mainbut_until_pressed();

		dac_update_task.stop();

		for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
			controls.SetEncoderLed(i, Palette::black);
		}
	}
};

} // namespace Catalyst2::HWTests
