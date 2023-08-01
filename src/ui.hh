#pragma once
#include "conf/model.hh"
#include "controls.hh"
#include "outputs.hh"
#include "params.hh"

namespace Catalyst2
{

class UI {
	Controls &controls;
	Params &params;
	Outputs outputs;

public:
	UI(Controls &controls, Params &params)
		: controls{controls}
		, params{params}
	{}

	void start()
	{
		controls.start();
	}

	void update()
	{
		controls.update();
		// Check controls and update params:
		// example;
		//  if (...)
		// 	    auto value_change = controls.read_encoder(i);
		//      params.scenes[cur_bank][i] += value_change;
		//      or: param.change_scene_chan(i, value_changed);
		//
		//  if (controls.alt_button.is_pressed() && controls.read_encoder(SeqLength).motion)
		//  	params.seq.length(...);
	}

	void set_outputs(Model::OutputBuffer &outs)
	{
		outputs.write(outs);

		// Then display outs on encoders, depending on the mode
		// if (controls.set_encoder_leds(...)
	}

private:
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
