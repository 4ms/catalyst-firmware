#pragma once

#include "channel.hh"
#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "debug.hh"
#include "drivers/adc_builtin.hh"
#include "drivers/debounced_switch.hh"
#include "drivers/led_driver_lp5024.hh"
#include "drivers/muxed_io.hh"
#include "muxed_button.hh"
#include "outputs.hh"
#include "saved_settings.hh"
#include "util/colors.hh"
#include "util/countzip.hh"
#include "util/filter.hh"
#include <cmath>
#include <optional>

namespace Catalyst2
{

class Controls {

	static inline std::array<uint16_t, Board::NumAdcs> adc_buffer;
	mdrivlib::AdcDmaPeriph<Board::AdcConf> adc_dma{adc_buffer, Board::AdcChans};

	CascadingFilter<uint16_t, SmoothOversampler<64, uint16_t>, HysteresisFilter<2, 1>> sliderf;
	CascadingFilter<uint16_t, SmoothOversampler<64, uint16_t>, HysteresisFilter<2, 1>> cv;

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
		MuxedButton clocksense{Board::Buttons::ClockJackSense};
	};
	// Jacks
	struct Jacks {
		Board::TrigJack trig;
		Board::ResetJack reset;
	};

	// Mux
	MuxedIO<Board::MuxConf> muxio;

	// LEDs
	mdrivlib::I2CPeriph led_driver_i2c{Board::LedDriverConf};
	mdrivlib::LP5024::Device led_driver{led_driver_i2c, Board::LedDriverAddr};

	std::array<uint8_t, Model::NumChans> button_led_duty{};
	std::array<Color, Model::NumChans> rgb_leds;
	Color::Adjustment global_brightness{128, 128, 128}; // 64, 64, 64};

	mdrivlib::Timekeeper encoder_led_update_task;
	mdrivlib::Timekeeper muxio_update_task;
	Board::PlayLed playled;
	bool leds_ready_flag = false;

	Outputs outputs;

public:
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
	Buttons button;
	Jacks jack;

	Controls() {
		Model::Output::Buffer zeros{Channel::Output::from_volts(0)};
		outputs.write(zeros);

		// 3.8%: 60Hz
		encoder_led_update_task.init(Board::encoder_led_task, [this]() { WriteToEncoderLeds(); });
		// 4.6%: 16kHz
		muxio_update_task.init(Board::muxio_conf, [this]() { UpdateMuxio(); });
	}

	void Start() {
		encoder_led_update_task.start();
		muxio_update_task.start();

		// 0.5us, 300kHz with interruptions
		adc_dma.register_callback([this] {
			constexpr auto slider_adc_chan = std::to_underlying(Model::AdcElement::Slider);
			sliderf.add_val(adc_buffer[slider_adc_chan]);

			constexpr auto cv_adc_chan = std::to_underlying(Model::AdcElement::CVJack);
			cv.add_val(adc_buffer[cv_adc_chan]);
		});
		adc_dma.start();

		if (!led_driver.init()) {
			// __BKPT();
		}
		// long delay to let the muxio run a few times so buttons checked on start up will be accurate
		Delay(64);
	}

	uint16_t ReadSlider() {
		auto val = std::clamp(sliderf.val(), Board::MinSliderVal, Board::MaxSliderVal);
		return MathTools::map_value(val, Board::MinSliderVal, Board::MaxSliderVal, 4095.9f, 0.f);
	}

	uint16_t ReadCv() {
		return cv.val();
	}

	void SetEncoderLed(unsigned led, Color color) {
		if (led >= Board::EncLedMap.size()) {
			return;
		}
		auto idx = Board::EncLedMap[led];
		rgb_leds[idx] = color.adjust(global_brightness);
	}

	void SetButtonLed(unsigned led, float intensity) {
		static constexpr std::array<uint8_t, 32> lut = {0, 0, 1, 1, 1, 1,  1,  1,  1,  2,  2,  2,  3,  3,  4,  4,
														5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 23, 25, 28, 32};
		intensity = std::clamp(0.f, .99f, intensity);
		button_led_duty[led] = lut[intensity * lut.size()];
	}

	void SetButtonLed(unsigned led, bool on) {
		button_led_duty[led] = on ? 32 : 0;
	}

	void SetPlayLed(bool on) {
		playled.set(on);
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
	bool LedsReady() {
		const auto ret = leds_ready_flag;
		leds_ready_flag = false;
		return ret;
	}
	void Delay(uint32_t ms) {
		HAL_Delay(ms);
	}
	void Write(const Model::Output::Buffer &outs) {
		outputs.write(outs);
	}
	static uint32_t TimeNow() {
		return HAL_GetTick();
	}

private:
	unsigned cur_encoder_led = 0;
	void WriteToEncoderLeds() {
		// about 120us to write one LED
		const std::span<const uint8_t, 3> raw_led_data(reinterpret_cast<uint8_t *>(rgb_leds.data() + cur_encoder_led),
													   3);
		led_driver.set_rgb_led(cur_encoder_led, raw_led_data);
		if (++cur_encoder_led >= Model::NumChans) {
			cur_encoder_led = 0;
			leds_ready_flag = true;
		}
	}
	void UpdateMuxio() {
		static uint8_t cnt = 0;
		auto button_leds = 0u;
		for (auto x = 0u; x < Model::NumChans; x++) {
			auto led = button_led_duty[x];
			if (cnt < led) {
				button_leds |= 1ul << Board::ButtonLedMap[x];
			}
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
	void UpdateButtons(uint32_t raw_mux_read) {
		for (auto &but : button.scene) {
			but.update(raw_mux_read);
		}
		button.shift.update(raw_mux_read);
		button.morph.update(raw_mux_read);
		button.bank.update(raw_mux_read);
		button.fine.update(raw_mux_read);
		button.add.update(raw_mux_read);
		button.play.update(raw_mux_read);
		button.clocksense.update(raw_mux_read);
	}
};
} // namespace Catalyst2
