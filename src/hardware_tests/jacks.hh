#pragma once
#include "conf/board_conf.hh"
#include "conf/palette.hh"
#include "hardware_tests/util.hh"
#include "libhwtests/CodecCallbacks.hh"
#include "libhwtests/GateInChecker.hh"

namespace Catalyst2::HWTests
{

struct TestJacks : IGateInChecker {
	Controls &controls;
	ManualValue pulse;
	Model::Output::Buffer outs;
	static constexpr auto SampleRate = 2000u;
	static constexpr auto NumJacks = 2u;

	uint8_t enc_map[NumJacks]{0, 3};

	TestJacks(Controls &controls)
		: IGateInChecker{NumJacks}
		, controls{controls} {
		reset();
		set_num_toggles(10);
		pulse.set_val(0);

		for (auto i : {0, 1, 2, 3, 4, 5, 6, 7}) {
			controls.SetButtonLed(i, false);
			controls.SetEncoderLed(i, Palette::off);
		}

		mdrivlib::Timekeeper dac_update_task{
			{
				.TIMx = TIM9,
				.period_ns = mdrivlib::TimekeeperConfig::Hz(SampleRate),
				.priority1 = 0,
				.priority2 = 1,
			},
			[this]() {
				outs[0] = pulse.update();
				this->controls.Write(outs);
			},
		};
		dac_update_task.start();
	}

	bool read_gate(uint8_t channel) override {
		if (channel == 0)
			return UtilIF::controls->jack.trig.is_high();

		if (channel == 1)
			return UtilIF::controls->jack.reset.is_high();

		return false;
	}

	void set_test_signal(bool newstate) override {
		HAL_Delay(2);
		pulse.set_val(newstate ? Channel::from_volts(5.f) : Channel::from_volts(0.f));
		HAL_Delay(2);
	}

	void set_error_indicator(uint8_t channel, ErrorType err) override {
		UtilIF::controls->SetEncoderLed(7, err == ErrorType::None ? Palette::off : Palette::red);
	}

	void set_indicator(uint8_t indicator_num, bool newstate) override {
		if (indicator_num >= NumJacks)
			return;
		UtilIF::controls->SetEncoderLed(enc_map[indicator_num], newstate ? Palette::green : Palette::off);
	}

	void signal_jack_done(uint8_t chan) override {
		if (chan >= NumJacks)
			return;
		UtilIF::controls->SetEncoderLed(enc_map[chan], Palette::blue);
	}

	bool is_ready_to_read_jack(uint8_t chan) override {
		return true;
	}
};
} // namespace Catalyst2::HWTests
