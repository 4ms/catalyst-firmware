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
	bool display_output = false;
	bool inserting = false;

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
		auto slider = controls.read_slider();
		params.morph_step = slider / 4096.f;
	}

	void update_cv()
	{
		auto cv = controls.read_cv() / (4096.f);
		params.cv_offset = cv;
	}

	// DG: This is a big function,already getting complex and there's more to add...
	// maybe needs its own file, maybe its own class, or maybe still part of ui, but in its own .cc file

	// Might help to abstract out the iteration of scene_buttons, since it's done so many times. Or just do it once to
	// see which one is pressed

	// Seperating by number of buttons down has pros and cons, but one con is for example,
	// if we want to have alt + encoder[6] randomize the current scene if no scene button is pressed,
	// then that has to appear in the down_count==1 and ==2 blocks.
	void update_mode_macro_classic()
	{
		controls.clear_button_leds();
		controls.set_all_encoder_leds(Palette::off);
		display_output = true;

		auto current_pos = params.morph_step;
		params.pathway.update(current_pos);

		if (controls.a_button.is_high() && controls.b_button.is_high()) {
			params.pathway.clear_scenes();
		}

		if (controls.a_button.just_went_high()) {
			inserting = true;
		} else if (controls.a_button.just_went_low()) {
			inserting = false;
		}

		if (controls.a_button.is_high()) {
			display_output = false;
			encoder_display_pathway_size();
			if (inserting == true) {
				if (params.pathway.on_a_scene) {
					scene_button_just_went_low([&](unsigned scene) {
						params.pathway.replace_scene(scene);
						inserting = false;
					});
				} else {
					scene_button_just_went_low([&](unsigned scene) {
						params.pathway.insert_scene(scene);
						inserting = false;
					});
				}
			} else {
				scene_button_just_went_low([&](unsigned scene) { params.pathway.insert_scene(scene, true); });
			}
		}

		get_encoder([&](int inc, unsigned chan) {
			get_scene_context([&](Pathway::SceneId scene) { params.banks.inc_chan(scene, chan, inc << 11); });
		});

		get_scene_context([&](Pathway::SceneId scene) { controls.set_button_led(scene, true); });
	}

	// what should this be named?
	void get_scene_context(auto f)
	{
		if (scene_button_high([&](unsigned chan) { f(chan); }))
			return;

		if (params.pathway.on_a_scene) {
			f(params.pathway.scene_nearest());
		} else {
			f(params.pathway.scene_left());
			f(params.pathway.scene_right());
		}
	}

	void scene_button_just_went_low(auto f)
	{
		for (auto [i, butt] : countzip(controls.scene_buttons)) {
			if (butt.just_went_low()) {
				f(i);
			}
		}
	}

	bool scene_button_high(auto f)
	{
		auto ret = false;

		for (auto [i, butt] : countzip(controls.scene_buttons)) {
			if (butt.is_high()) {
				f(i);
				ret = true;
			}
		}
		return ret;
	}

	void get_encoder(auto f)
	{
		for (auto [i, enc] : countzip(controls.encoders)) {
			auto inc = enc.read();
			if (inc)
				f(inc, i);
		}
	}

	void encoder_display_pathway_size()
	{
		auto r = params.pathway.size();
		auto phase = 1.f / (params.pathway.MaxPoints / static_cast<float>(r));

		while (r > 8)
			r -= 8;

		encoder_display_count(Palette::green.blend(Palette::red, phase), r);
	}

	void encoder_display_count(Color c, unsigned count)
	{
		if (count > Model::NumChans)
			return;

		for (auto i = 0u; i < count; i++)
			controls.set_encoder_led(i, c);
	}

	Color encoder_blend(uint16_t phase_)
	{
		constexpr auto zero_v = ChannelValue::from_volts(0.f);
		constexpr auto five_v = ChannelValue::from_volts(5.f);

		auto phase = static_cast<float>(phase_);

		if (phase < zero_v) {
			float temp = phase / zero_v;
			uint8_t o = temp * 255.f;
			return Palette::red.blend(Palette::yellow, o);
		} else if (phase < five_v) {
			phase -= zero_v;
			float temp = phase / zero_v;
			uint8_t o = temp * 255.f;
			return Palette::yellow.blend(Palette::green, o);
		} else {
			phase -= five_v;
			float temp = phase / zero_v;
			uint8_t o = temp * 255.f;
			return Palette::green.blend(Palette::blue, o);
		}
	}

	void update_mode_switch()
	{
		if (controls.mode_switch.is_high())
			params.mode = Params::Mode::Sequencer;
		else
			params.mode = Params::Mode::Macro;
	}

	mdrivlib::Timekeeper encoder_led_update_task;
};

} // namespace Catalyst2
