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
	enum class State {
		MacroIdle,
		MacroAlt,
		MacroAB,
		MacroBank,
	};
	State state = State::MacroIdle;
	Controls controls;
	Params &params;
	InternalClock<Board::cv_stream_hz> intclock;
	Outputs outputs;
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

			// we can initialize states here.
			if (controls.alt_button.just_went_high()) {
				state = State::MacroAlt;
			} else if (controls.a_button.just_went_high() || controls.b_button.just_went_high()) {
				state = State::MacroAB;
				// clear the falling edge states
				scene_button_just_went_low([](Pathway::SceneId garb) {});
			} else if (controls.bank_button.just_went_high()) {
				state = State::MacroBank;
			}

			// common to all states
			controls.clear_button_leds();

			auto current_pos = params.slider_pos;
			params.pathway.update(current_pos);

			switch (state) {
				case State::MacroAlt:
					macro_state_alt();
					break;
				case State::MacroAB:
					macro_state_ab();
					break;
				case State::MacroBank:
					macro_state_bank();
					break;
				case State::MacroIdle:
					macro_state_idle();
					break;
			}

		} else {
		}

		// sequencer mode
	}

	void update_slider()
	{
		auto slider = controls.read_slider();
		params.slider_pos = slider / 4096.f;
	}

	void update_cv()
	{
		auto cv = controls.read_cv() / (4096.f);
		params.cv_offset = cv;
	}

	void macro_state_idle()
	{
		// controls.set_all_encoder_leds(Palette::off);

		Pathway::SceneId scene_to_display = 0xff;

		if (scene_button_high([&](Pathway::SceneId scene) {
				controls.set_button_led(scene, true);
				scene_to_display = scene;
			}))
		{
			// do this once regardless if any amount of buttons are pressed.
			display_output = false;

			// is this too redundant?
			get_encoder([&](int inc, unsigned chan) {
				scene_button_high([&](Pathway::SceneId scene) { params.banks.inc_chan(scene, chan, inc << 11); });
			});

			// TODO: somehow this scene needs to be sent out the output

			encoder_display_scene(scene_to_display);
		} else {
			// do this if no scene buttons were pressed.
			display_output = true;

			// display the current scene based on slider pos
			get_scene_context([&](Pathway::SceneId scene) { controls.set_button_led(scene, true); });

			get_encoder([&](int inc, unsigned chan) {
				get_scene_context([&](Pathway::SceneId scene) { params.banks.inc_chan(scene, chan, inc << 11); });
			});
		}
	}

	void macro_state_ab()
	{
		static bool first = true;
		display_output = false;
		controls.set_all_encoder_leds(Palette::off);
		encoder_display_pathway_size();

		get_scene_context([&](Pathway::SceneId scene) { controls.set_button_led(scene, true); });

		/* a and b buttons*/
		if (controls.a_button.is_high()) {

			if (controls.b_button.is_high()) {
				params.pathway.clear_scenes();
			} else {
				scene_button_just_went_low([&](unsigned scene) {
					if (first) {
						if (params.pathway.on_a_scene()) {
							params.pathway.replace_scene(scene);
						} else {
							params.pathway.insert_scene(scene, false);
						}
						first = false;
					} else {
						params.pathway.insert_scene(scene, true);
					}
				});
			}
		} else if (controls.b_button.is_high()) {
			// removing a scene is not nearly as intuitive as inserting them
			if (params.pathway.on_a_scene()) {
				scene_button_just_went_low([&](unsigned scene) {
					if (scene == params.pathway.scene_nearest())
						params.pathway.remove_scene();
				});
			}
		}

		if (!controls.a_button.is_high() && !controls.b_button.is_high()) {
			state = State::MacroIdle;
			first = true;
		}
	}

	void macro_state_alt()
	{
		if (!controls.alt_button.is_high()) {
			state = State::MacroIdle;
		}
	}

	void macro_state_bank()
	{
		scene_button_high([&](unsigned chan) { params.banks.sel_bank(chan); });
		controls.set_button_led(params.banks.get_sel_bank(), true);

		if (!controls.bank_button.is_high()) {
			state = State::MacroIdle;
		}
	}

	// what should this be named?
	void get_scene_context(auto f)
	{
		if (params.pathway.on_a_scene()) {
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

	/// @brief runs a function for each scene button currently pressed.
	/// @param f
	/// @return true if any scene buttons were pressed
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

	void encoder_display_scene(Pathway::SceneId scene)
	{
		for (auto i = 0u; i < Model::NumChans; i++) {
			auto temp = params.banks.get_chan(scene, i);
			controls.set_encoder_led(i, encoder_blend(temp));
		}
	}

	Color encoder_blend(uint16_t level)
	{
		constexpr auto zero_v = ChannelValue::from_volts(0.f);
		constexpr auto five_v = ChannelValue::from_volts(5.f);

		auto phase = static_cast<float>(level);

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
