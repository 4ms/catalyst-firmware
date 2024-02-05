#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"

namespace Catalyst2::HWTests
{

struct TestEncoders {
	Controls &controls;
	std::array<int32_t, 8> rotvals{0};
	enum class State { Initial, CheckingCCW, CheckingCW, Done } state = State::Initial;

	TestEncoders(Controls &controls)
		: controls{controls} {
	}

	void run_test() {

		for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
			controls.SetEncoderLed(i, Palette::yellow);
		}

		using enum State;
		state = CheckingCCW;

		while (state != Done) {
			for (auto i = 0u; i < controls.encoders.size(); i++) {
				const auto dir = controls.encoders[i].read();
				rotvals[i] += dir;

				if (state == CheckingCCW && (rotvals[i] < 0)) {
					controls.SetEncoderLed(i, Palette::red);
				} else if (state == CheckingCW && (rotvals[i] > 0)) {
					controls.SetEncoderLed(i, Palette::blue);
				}
			}

			if (state == CheckingCCW) {
				auto num_ccw = std::count_if(rotvals.begin(), rotvals.end(), [](auto rot) { return rot < 0; });
				if (num_ccw == rotvals.size()) {
					state = CheckingCW;
					for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
						rotvals[i] = 0;
						controls.SetEncoderLed(i, Palette::yellow);
					}
				}
			} else if (state == CheckingCW) {
				auto num_cw = std::count_if(rotvals.begin(), rotvals.end(), [](auto rot) { return rot > 0; });
				if (num_cw == rotvals.size())
					state = Done;
			}

			HAL_Delay(10);

			if (Util::main_button_pressed())
				break;
		}

		while (Util::main_button_pressed())
			;

		for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
			controls.SetEncoderLed(i, Palette::green);
		}
	}
};

} // namespace Catalyst2::HWTests
