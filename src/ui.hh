#pragma once
#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "intclock.hh"
#include "outputs.hh"
#include "params.hh"
#include "util/countzip.hh"

namespace Catalyst2
{

class UI {
	Controls controls;
	Params &params;
	InternalClock<Board::cv_stream_hz> intclock;
	Outputs outputs;
	unsigned buttons_down;
	bool display_output = false;

public:
	UI(Params &params)
		: params{params}
	{
		encoder_led_update_task.init(Board::encoder_led_task, [&]() { controls.write_to_encoder_leds(); });
	}

	void start()
	{
		encoder_led_update_task.start();
		controls.start();
	}

	void update()
	{
		controls.update();

		update_mode();
		update_slider();
		update_cv();

		// for testing clock
		if (update_sequencer()) {
			// controls.toggle_button_led(0);
			/* sequencer.step()?? */
		}

		// TODO
		// Check controls and update params:
		// example;
		//  if (...)
		// 	    auto value_change = controls.read_encoder(chan);
		//      params.scenes[cur_bank][chan] += value_change;
		//      or: param.change_scene_chan(chan, value_changed);
		//
		//  if (controls.alt_button.is_pressed() && controls.read_encoder(SeqLength).motion)
		//  	params.seq.set_length(...);
	}

	void set_outputs(Model::OutputBuffer &outs)
	{
		outputs.write(outs);
		if (display_output) {
			for (auto [chan, val] : countzip(outs)) {
				controls.set_encoder_led(chan, encoder_blend(val));
			}
		}
	}

private:
	void update_mode()
	{
		update_mode_switch();

		if (params.mode == Params::Mode::Macro) {
			switch (params.macromode) {
				case Params::MacroMode::Classic:
					update_mode_macro_classic();
					break;
				case Params::MacroMode::OutofOrderSequence:
					break;
				case Params::MacroMode::Pathway:
					break;
			}
		} else {
			switch (params.seqmode) {
				case Params::SeqMode::Multi:
					break;
				case Params::SeqMode::Single:
					break;
			}
		}

		// sequencer mode
	}

	void update_slider()
	{
		params.morph_step = controls.read_slider() / 4095.f;
	}

	void update_cv()
	{
		auto cv = controls.read_cv() / (4095.f * .5f);
		params.cv_offset = cv - 1.f;
	}

	void update_mode_macro_classic()
	{
		const auto down_count = controls.button_high_count();
		controls.clear_button_leds();
		controls.set_all_encoder_leds(Palette::off);
		display_output = false;

		if (down_count == 0) {
			controls.clear_encoders_state();
			display_output = true;
		} else if (down_count == 1) {
			bool bbut = controls.b_button.is_high();
			if (controls.a_button.is_high() || bbut) {
				controls.set_button_led(params.part.get_sel_scene(bbut), true);

				for (auto [chan, enc] : countzip(controls.encoders)) {
					params.part.inc_chan(bbut, chan, enc.read() << 7);
					controls.set_encoder_led(chan, encoder_blend(params.part.get_chan(bbut, chan)));
				}
			} else if (controls.bank_button.is_high()) {
				controls.set_button_led(params.part.get_sel_bank(), true);
			} else {
				/*other buttons*/
			}
		} else if (down_count == 2) {
			for (auto [chan, button] : countzip(controls.scene_buttons)) {
				if (button.is_high()) {
					bool bbut = controls.b_button.is_high();
					if (controls.a_button.is_high() || bbut) {
						controls.set_button_led(params.part.get_sel_scene(bbut), true);
						for (auto [chan, val] : countzip(params.part.get_scene(bbut).chans)) {
							controls.set_encoder_led(chan, encoder_blend(val));
						}
						params.part.sel_scene(bbut, chan);
					} else if (controls.bank_button.is_high()) {
						params.part.sel_bank(chan);
					} else {
						// invalid combo
					}
				}
			}
		}
	}

	Color encoder_blend(uint16_t phase)
	{
		return Palette::red.blend(Palette::blue, static_cast<uint8_t>(phase >> 8));
	}

	void update_mode_switch()
	{
		if (controls.mode_switch.is_high())
			params.mode = Params::Mode::Sequencer;
		else
			params.mode = Params::Mode::Macro;
	}

	bool update_sequencer()
	{
		bool pulse = false;

		if (controls.trig_jack_sense.is_high()) {
			intclock.update();
			pulse = intclock.step();
		} else {
			pulse = controls.trig_jack.just_went_high();
		}

		return params.mode == Params::Mode::Sequencer ? pulse : false;
	}

	mdrivlib::Timekeeper encoder_led_update_task;

	// TODO:remove this if not using
	// struct PotState {
	// 	int16_t cur_val = 0;
	// 	int16_t prev_val = 0;
	// 	int16_t track_moving_ctr = 0;
	// 	int16_t delta = 0;
	// 	bool moved = false;
	// 	bool moved_while_button_down = false; // can make this an array if need to track all buttons+slider
	// } slider_state;

	struct EncoderState {};
};

} // namespace Catalyst2
