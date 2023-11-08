#pragma once

#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "debug.hh"
#include "drivers/adc_builtin.hh"
#include "drivers/debounced_switch.hh"
#include "drivers/led_driver_lp5024.hh"
#include "drivers/muxed_io.hh"
#include "muxed_button.hh"
#include "util/colors.hh"
#include "util/filter.hh"
#include <cmath>
#include <optional>
#include <util/countzip.hh>

namespace Catalyst2
{

class Controls {
	static inline std::array<uint16_t, Board::NumAdcs> adc_buffer;
	mdrivlib::AdcDmaPeriph<Board::AdcConf> adc_dma{adc_buffer, Board::AdcChans};
	std::array<Oversampler<256, uint16_t>, Board::NumAdcs> analog;

	struct Buttons {
		std::array<MuxedButton, Model::NumChans> scene{
			Board::Buttons::SceneMap[0],
			Board::Buttons::SceneMap[1],
			Board::Buttons::SceneMap[2],
			Board::Buttons::SceneMap[3],
			Board::Buttons::SceneMap[4],
			Board::Buttons::SceneMap[5],
			Board::Buttons::SceneMap[6],
			Board::Buttons::SceneMap[7],
		};
		MuxedButton shift{Board::Buttons::Shift};
		MuxedButton morph{Board::Buttons::Morph};
		MuxedButton bank{Board::Buttons::Bank};
		MuxedButton fine{Board::Buttons::Fine};
		MuxedButton add{Board::Buttons::Add};
		MuxedButton play{Board::Buttons::Play};
	};
	// Switches
	struct Toggles {
		MuxedButton mode{Board::ModeSwitch};
		MuxedButton trig_sense{Board::TrigJackSense};
	};
	// Jacks
	struct Jacks {
		Board::TrigJack trig;
		Board::ResetJack reset;
	};
	// Encoders
	std::array<mdrivlib::RotaryEncoder, Model::NumChans> encoders{{
		{Board::Enc1A, Board::Enc1B, Board::EncStepSize},
		{Board::Enc2A, Board::Enc2B, Board::EncStepSize},
		{Board::Enc3A, Board::Enc3B, Board::EncStepSize},
		{Board::Enc4A, Board::Enc4B, Board::EncStepSize},
		{Board::Enc5A, Board::Enc5B, Board::EncStepSize},
		{Board::Enc6A, Board::Enc6B, Board::EncStepSize},
		{Board::Enc7A, Board::Enc7B, Board::EncStepSize},
		{Board::Enc8A, Board::Enc8B, Board::EncStepSize},
	}};

public:
	Buttons button;
	Toggles toggle;
	Jacks jack;

	Controls() = default;

	// TODO:move to shared file
	std::optional<uint8_t> YoungestSceneButton() {
		auto age = 0xffffffffu;
		uint8_t youngest = 0xff;

		for (auto [i, b] : countzip(button.scene)) {
			if (!b.is_high())
				continue;

			if (b.time_high > age)
				continue;

			age = b.time_high;
			youngest = i;
		}
		if (youngest == 0xff)
			return std::nullopt;

		return youngest;
	}

	// unused
	int32_t GetEncoder(uint8_t idx) {
		return encoders[idx].read();
	}

	void ForEachEncoderInc(auto func) {
		for (auto [i, enc] : countzip(encoders)) {
			auto inc = enc.read();
			if (inc)
				func(i, inc);
		}
	}

	uint16_t ReadSlider() {
		constexpr auto adc_chan_num = std::to_underlying(Model::AdcElement::Slider);
		return (1ul << 12) - 1 - analog[adc_chan_num].val();
	}

	uint16_t ReadCv() {
		constexpr auto adc_chan_num = std::to_underlying(Model::AdcElement::CVJack);
		return analog[adc_chan_num].val();
	}

	// TODO: move to a file shared by all projects
	void SetEncoderLedsCount(uint8_t count, uint8_t offset, Color c) {
		for (auto i = 0u; i < count; i++)
			SetEncoderLed((i + offset) & 7, c);

		for (auto i = 0u; i < Model::NumChans - count; i++)
			SetEncoderLed((count + i + offset) & 7, Colors::off);
	}

	// TODO: move to a file shared by all projects
	void SetEncoderLedsAddition(uint8_t num, Color c) {
		static constexpr auto max_val = [] {
			uint8_t out = 0;
			for (auto i = 1u; i <= Model::NumChans; i++)
				out += i;
			return out;
		}();

		if (num > max_val)
			return;

		uint8_t t = Model::NumChans;

		while (num >= t) {
			num -= t;
			t -= 1;
			SetEncoderLed(t, c);
		}
		SetEncoderLed(num - 1, c);
	}

	void SetEncoderLed(unsigned led, Color color) {
		if (led >= Board::EncLedMap.size())
			return;

		auto idx = Board::EncLedMap[led];
		rgb_leds[idx] = color;
	}

	// TODO: move to a file shared by all projects
	void SetButtonLedsCount(uint8_t count, bool on) {
		for (auto i = 0u; i < count; i++)
			SetButtonLed(i, on);
	}

	void SetButtonLed(unsigned led, float intensity) {
		static constexpr std::array<uint8_t, 32> lut = {0, 0, 1, 1, 1, 1,  1,  1,  1,  2,  2,  2,  3,  3,  4,  4,
														5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 23, 25, 28, 32};
		intensity = std::clamp<float>(0, .99, intensity);
		button_led_duty[led] = lut[intensity * lut.size()];
	}

	void SetButtonLed(unsigned led, bool on) {
		button_led_duty[led] = on ? 32 : 0;
	}

	void ClearButtonLeds() {
		for (auto &a : button_led_duty)
			a = 0;
	}

	void ClearEncoderLeds() {
		for (auto &led : rgb_leds)
			led = Colors::off;
	}

	void Update() {
		// TODO: double-check if this is concurrency-safe:
		// - update() might interrupt the read-modify-write that happens in set_button_led()
		// - update_buttons() might interrupt a button being read
		jack.trig.update();
		jack.reset.update();
		for (auto &enc : encoders) {
			enc.update();
		}
	}

	void UpdateMuxio() {
		static uint8_t cnt = 0;
		auto button_leds = 0u;
		for (auto x = 0u; x < Model::NumChans; x++) {
			auto led = button_led_duty[x];
			if (cnt < led)
				button_leds |= 1ul << Board::ButtonLedMap[x];
		}

		auto mux_read = muxio.step(button_leds);
		if (mux_read.has_value()) {
			UpdateButtons(mux_read.value());
			cnt++;
			if (cnt >= 32) {
				cnt = 0;
				UpdateButtons(mux_read.value());
			}
		}
	}

	void Start() {
		adc_dma.register_callback([this] {
			for (unsigned i = 0; auto &a : analog)
				a.add_val(adc_buffer[i++]);
		});
		adc_dma.start();
		if (!led_driver.init())
			__BKPT();
	}

	void WriteToEncoderLeds() {
		// Takes about 620us to write all LEDs
		const std::span<const uint8_t, 24> raw_led_data(reinterpret_cast<uint8_t *>(rgb_leds.data()), 24);
		led_driver.set_all_leds(raw_led_data);
	}

private:
	void UpdateButtons(uint32_t raw_mux_read) {
		for (auto &but : button.scene)
			but.update(raw_mux_read);

		button.shift.update(raw_mux_read);
		button.morph.update(raw_mux_read);
		button.bank.update(raw_mux_read);
		button.fine.update(raw_mux_read);
		button.add.update(raw_mux_read);
		button.play.update(raw_mux_read);

		toggle.mode.update(raw_mux_read);
		toggle.trig_sense.update(raw_mux_read);
	}

	// Mux
	MuxedIO<Board::MuxConf> muxio;

	// LEDs (needs work)
	mdrivlib::I2CPeriph led_driver_i2c{Board::LedDriverConf};
	mdrivlib::LP5024::Device led_driver{led_driver_i2c, Board::LedDriverAddr};

	uint32_t button_leds = 0;
	std::array<uint8_t, Model::NumChans> button_led_duty;
	std::array<Color, Model::NumChans> rgb_leds;
};
} // namespace Catalyst2
