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
	std::array<MuxedButton, Model::NumChans> scene_buttons{11, 8, 7, 5, 9, 10, 4, 6};
	MuxedButton alt_button{0};
	MuxedButton latch_button{12};
	MuxedButton bank_button{2};
	MuxedButton a_button{14};
	MuxedButton b_button{1};
	MuxedButton play_button{15};

	// Switches
	MuxedButton mode_switch{3};
	MuxedButton trig_jack_sense{13}; // Sense pin detects if jack is patched or not

	Toggler &get_scene_button(unsigned idx)
	{
		if (idx >= Model::NumChans)
			idx = 0;
		return scene_buttons[idx].button;
	}

	uint16_t read_slider()
	{
		constexpr auto adc_chan_num = std::to_underlying(Model::AdcElement::Slider);
		return analog[adc_chan_num].val();
	}

	uint16_t read_cv()
	{
		constexpr auto adc_chan_num = std::to_underlying(Model::AdcElement::CVJack);
		return analog[adc_chan_num].val();
	}

	Model::ModeSwitch read_mode_switch()
	{
		using enum Model::ModeSwitch;
		return mode_switch.button.is_pressed() ? Sequence : Macro;
	}

	// TODO: Add More functions as needed: read_scene_button(num), read_encoder() etc.
	///

	void set_encoder_led(unsigned led, Color color)
	{
		if (led >= rgb_leds.size())
			return;

		rgb_leds[led] = color;
	}

	void set_button_led(unsigned led, bool on)
	{
		if (led >= Model::NumChans)
			return;

		if (on)
			button_leds |= (1 << led);
		else
			button_leds &= ~(1 << led);
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

	void update()
	{
		trig_jack.update();
		reset_jack.update();
		for (auto &enc : encoders) {
			enc.update();
		}

		// TODO: maybe update_mux() should happen on a separate timer so we don't get flicker?
		// We might have concurrency issues though...
		update_mux();

		// TODO: maybe call write_to_encoder_leds() on a separate timer also?
		//?? write_to_encoder_leds()
	}

	void update_mux()
	{
		auto raw_mux_read = muxio.readwrite(button_leds);
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

	void write_to_encoder_leds()
	{
		// Takes about 2.3ms to write all LEDs
		Debug::Pin0::high();
		const std::span<const uint8_t, 24> raw_led_data(reinterpret_cast<uint8_t *>(rgb_leds.data()), 24);
		led_driver.set_all_leds(raw_led_data);
		Debug::Pin0::low();
	}

private:
	// Mux
	MuxedIO<Board::MuxConf> muxio;

	// LEDs (needs work)
	mdrivlib::I2CPeriph led_driver_i2c{Board::LedDriverConf};
	mdrivlib::LP5024::Device led_driver{led_driver_i2c, Board::LedDriverAddr};

	uint32_t button_leds = 0;
	std::array<Color, Model::NumChans> rgb_leds;
};
} // namespace Catalyst2
