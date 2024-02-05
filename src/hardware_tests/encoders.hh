#pragma once
#include "conf/palette.hh"
#include "controls.hh"
#include "hardware_tests/util.hh"

namespace Catalyst2::HWTests
{
std::array<volatile int32_t, 8> rotvals{0};

struct TestEncoders {
	Controls &controls;
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
				auto dir = controls.encoders[i].read();
				rotvals[i] += dir;
				if (state == CheckingCW) {
					if (rotvals[i] < 0)
						rotvals[i] = 0;
				}

				if (dir < 0) {
					controls.SetEncoderLed(i, Palette::red);
				} else if (dir > 0) {
					controls.SetEncoderLed(i, Palette::blue);
				}
			}

			if (state == CheckingCCW) {
				auto sum = 0u;
				for (auto [i, rot] : enumerate(rotvals)) {
					if (rot < 0)
						sum++;
				}
				if (sum == rotvals.size()) {
					state = CheckingCW;
					for (auto [i, rot] : enumerate(rotvals)) {
						rot = 0;
						controls.SetEncoderLed(i, Palette::yellow);
					}
				}

			} else if (state == CheckingCW) {
				auto sum = 0u;
				for (auto [i, rot] : enumerate(rotvals)) {
					if (rot > 0)
						sum++;
				}
				if (sum == rotvals.size())
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
