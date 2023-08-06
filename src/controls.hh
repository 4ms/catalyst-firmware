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

namespace Catalyst2
{

class Controls {
	static inline std::array<uint16_t, Board::NumAdcs> adc_buffer;
	mdrivlib::AdcDmaPeriph<Board::AdcConf> adc_dma{adc_buffer, Board::AdcChans};
	std::array<Oversampler<256, uint16_t>, Board::NumAdcs> analog;

public:
	Controls() = default;

	// Jacks
	Board::TrigJack trig_jack;
	Board::ResetJack reset_jack;

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

	// Buttons
	// TODO: put these indices into board_conf.hh
	std::array<MuxedButton, Model::NumChans> scene_buttons{11, 8, 7, 5, 9, 10, 4, 6};
	MuxedButton alt_button{0};
	MuxedButton latch_button{12};
	MuxedButton bank_button{2};
	MuxedButton a_button{14};
	MuxedButton b_button{1};
	MuxedButton play_button{15};
	// static constexpr uint16_t ui_button_mask = 0b1101'1111'1111'0111;

	// Switches
	MuxedButton mode_switch{3};
	MuxedButton trig_jack_sense{13};

	auto &scene_button(unsigned idx)
	{
		if (idx >= Model::NumChans)
			idx = 0;
		return scene_buttons[idx];
	}

	auto button_high_count()
	{
		auto down_count = 0;

		for (auto &but : scene_buttons)
			down_count += but.is_high();

		down_count += alt_button.is_high();
		down_count += latch_button.is_high();
		down_count += bank_button.is_high();
		down_count += a_button.is_high();
		down_count += b_button.is_high();
		down_count += play_button.is_high();

		return down_count;
	}

	uint16_t read_slider()
	{
		constexpr auto adc_chan_num = std::to_underlying(Model::AdcElement::Slider);
		return (1ul << 12) - 1 - analog[adc_chan_num].val();
	}

	uint16_t read_cv()
	{
		constexpr auto adc_chan_num = std::to_underlying(Model::AdcElement::CVJack);
		return analog[adc_chan_num].val();
	}

	Model::ModeSwitch read_mode_switch()
	{
		using enum Model::ModeSwitch;
		return mode_switch.is_pressed() ? Sequence : Macro;
	}

	// TODO: Add More functions as needed: read_scene_button(num), read_encoder() etc.

	void set_encoder_led(unsigned led, Color color)
	{
		if (led >= Board::EncLedMap.size())
			return;

		auto idx = Board::EncLedMap[led];
		rgb_leds[idx] = color;
	}

	void set_all_encoder_leds(Color color)
	{
		for (auto &led : rgb_leds) {
			led = color;
		}
	}

	void set_button_led(unsigned led, bool on)
	{
		if (led >= Board::ButtonLedMap.size())
			return;

		auto bit = Board::ButtonLedMap[led];
		if (on)
			button_leds |= (1 << bit);
		else
			button_leds &= ~(1 << bit);
	}

	void set_button_led_masked(uint32_t mask, bool on)
	{
		if (on) {
			button_leds |= mask;
		} else {
			button_leds &= ~(mask);
		}
	}

	void clear_button_leds()
	{
		button_leds = 0;
	}

	bool get_button_led(unsigned led)
	{
		if (led >= Board::ButtonLedMap.size())
			return false;

		auto bit = Board::ButtonLedMap[led];

		return button_leds & (1 << bit);
	}

	void toggle_button_led(unsigned led)
	{
		auto temp = get_button_led(led);
		temp ^= 1;
		set_button_led(led, temp);
	}

	void start()
	{
		adc_dma.register_callback([this] {
			for (unsigned i = 0; auto &a : analog)
				a.add_val(adc_buffer[i++]);
		});
		adc_dma.start();
		if (!led_driver.init())
			__BKPT();
	}

	void clear_encoders_state()
	{
		for (auto &x : encoders)
			x.read();
	}

	void update()
	{
		// TODO: double-check if this is concurrency-safe:
		// - update() might interrupt the read-modify-write that happens in set_button_led()
		// - update_buttons() might interrupt a button being read
		// - do this routine first for the sake of non-flickering leds
		auto mux_read = muxio.step(button_leds);
		if (mux_read.has_value()) {
			update_buttons(mux_read.value());
		}

		trig_jack.update();
		reset_jack.update();
		for (auto &enc : encoders) {
			enc.update();
		}
	}

	void write_to_encoder_leds()
	{
		// Takes about 620us to write all LEDs
		const std::span<const uint8_t, 24> raw_led_data(reinterpret_cast<uint8_t *>(rgb_leds.data()), 24);
		led_driver.set_all_leds(raw_led_data);
	}

private:
	void update_buttons(uint32_t raw_mux_read)
	{
		for (auto &but : scene_buttons)
			but.update(raw_mux_read);

		alt_button.update(raw_mux_read);
		latch_button.update(raw_mux_read);
		bank_button.update(raw_mux_read);
		a_button.update(raw_mux_read);
		b_button.update(raw_mux_read);
		play_button.update(raw_mux_read);

		mode_switch.update(raw_mux_read);
		trig_jack_sense.update(raw_mux_read);
	}

	// Mux
	MuxedIO<Board::MuxConf> muxio;

	// LEDs (needs work)
	mdrivlib::I2CPeriph led_driver_i2c{Board::LedDriverConf};
	mdrivlib::LP5024::Device led_driver{led_driver_i2c, Board::LedDriverAddr};

	uint32_t button_leds = 0;
	std::array<Color, Model::NumChans> rgb_leds;
};
} // namespace Catalyst2
