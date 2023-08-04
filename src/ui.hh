#pragma once
#include "conf/model.hh"
#include "controls.hh"
#include "intclock.hh"
#include "outputs.hh"
#include "params.hh"

namespace Catalyst2
{

class UI {
	Controls controls;
	Params &params;
	InternalClock<Board::cv_stream_hz> intclock;
	Outputs outputs;

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

		// for testing clock
		params.mode = Params::Mode::Sequencer;
		if (update_sequencer()) {
			controls.toggle_button_led(0);
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

		// TODO: display outs on encoders, depending on the mode
	}

private:
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
	struct PotState {
		int16_t cur_val = 0;
		int16_t prev_val = 0;
		int16_t track_moving_ctr = 0;
		int16_t delta = 0;
		bool moved = false;
		bool moved_while_button_down = false; // can make this an array if need to track all buttons+slider
	} slider_state;
};

} // namespace Catalyst2
