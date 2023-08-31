#include "ui.hh"

namespace Catalyst2
{

void UI::update_mode()
{
	update_mode_switch();

	// common to all states
	controls.clear_button_leds();
	display_output = true;

	if (params.mode == Params::Mode::Macro) {

		switch (state) {
			case State::MacroAlt_GlobalInit:
				state = State::MacroAlt_Global;
			case State::MacroAlt_Global:
				macro_state_alt_global();
				break;

			case State::MacroAlt_SceneInit:
				state = State::MacroAlt_Scene;
			case State::MacroAlt_Scene:
				macro_state_alt_scene();
				break;

			case State::MacroABInit:
				controls.scene_buttons_clear_events();
				state = State::MacroAB;
			case State::MacroAB:
				macro_state_ab();
				break;

			case State::MacroBankInit:
				state = State::MacroBank;
			case State::MacroBank:
				global_state_bank();
				break;

			case State::MacroIdleInit:
				controls.b_button.clear_events();
				controls.bank_button.clear_events();
				state = State::MacroIdle;
			case State::MacroIdle:
				macro_state_idle();
				break;

			default:
				state = State::MacroIdleInit;
				break;
		}

		return;
	}

	// sequencer mode
	if (controls.reset_jack.just_went_high())
		params.seq.reset();
	seq_update_step();

	switch (state) {
		case State::SeqIdleInit:
			state = State::SeqIdle;
		case State::SeqIdle:
			seq_state_idle();
			break;

		case State::SeqAlt_ChannelInit:
			state = State::SeqAlt_Channel;
		case State::SeqAlt_Channel:
			seq_state_alt_channel();
			break;

		case State::SeqBankInit:
			state = State::SeqBank;
		case State::SeqBank:
			global_state_bank();
			break;

		default:
			state = State::SeqIdleInit;
			break;
	}
}

void UI::seq_state_idle()
{
	scene_button_just_went_low([&](unsigned chan) { params.seq.sel_chan(chan); });
	auto alt = controls.alt_button.is_high();

	if (params.seq.is_chan_selected()) {
		encoder_display_sequence();
		get_encoder(
			[&](int inc, unsigned scene) { params.banks.adj_chan(scene, params.seq.get_sel_chan(), inc, alt); });
	} else {
		;
	}

	if (alt) {
		if (controls.b_button.just_went_high()) {
			; // state = State::SeqAlt_GlobalInit;
		} else if (controls.bank_button.just_went_high()) {
			state = State::SeqAlt_ChannelInit;
		}
	} else if (controls.bank_button.is_high()) {
		state = State::SeqBankInit;
	}
}

void UI::seq_state_alt_channel()
{
	// force a channel to be selected.
	if (!params.seq.is_chan_selected())
		params.seq.sel_chan(0);
	// don't allow it to be unselected
	auto cur_chan = params.seq.get_sel_chan();
	scene_button_just_went_low([&](unsigned chan) {
		if (chan != cur_chan)
			params.seq.sel_chan(chan);
	});

	encoder_display_sequence_length();

	// seq_length
	auto inc = controls.encoders[5].read();
	params.seq.adj_length(cur_chan, inc);

	state = controls.alt_button.is_high() ? state : State::SeqIdleInit;
}

void UI::macro_state_idle()
{
	const auto alt = controls.alt_button.is_high();

	if (controls.play_button.just_went_high()) {
		if (alt) {
			recorder.record();
		} else {
			recorder.toggle();
		}
	}

	if (recorder.is_recordering()) {
		scene_button_display_recording();
		return;
	}

	if (controls.trig_jack.just_went_high() && recorder.size()) {
		recorder.play();
	}

	if (controls.reset_jack.just_went_high() && recorder.size()) {
		recorder.restart();
	}

	// display the last one pressed.
	Pathway::SceneId scene_to_display = 0xff;
	auto age = 0xffffffffu;

	if (scene_button_high([&](Pathway::SceneId scene) {
			controls.set_button_led(scene, true);

			if (controls.scene_buttons[scene].time_high < age) {
				age = controls.scene_buttons[scene].time_high;
				scene_to_display = scene;
			}
		}))
	{
		// do this once regardless if any amount of buttons are pressed.
		// is this too redundant?
		get_encoder([&](int inc, unsigned chan) {
			scene_button_high([&](Pathway::SceneId scene) { params.banks.adj_chan(scene, chan, inc, alt); });
		});

		// TODO: somehow this scene needs to be sent out the output
		encoder_display_scene(scene_to_display);

		return;
	}

	// do this if no scene buttons were pressed.
	// display the current scene
	scene_button_display_nearest();

	get_encoder([&](int inc, unsigned chan) {
		get_scene_context([&](Pathway::SceneId scene) { params.banks.adj_chan(scene, chan, inc, alt); });
	});

	// check state
	if (alt) {
		if (controls.b_button.just_went_high()) {
			state = State::MacroAlt_GlobalInit;
		} else if (controls.bank_button.just_went_high()) {
			state = State::MacroAlt_SceneInit;
		}
	} else if (controls.a_button.is_high() || controls.b_button.is_high()) {
		state = State::MacroABInit;
	} else if (controls.bank_button.is_high()) {
		state = State::MacroBankInit;
	}
}

void UI::macro_state_alt_global()
{
	display_output = false;
	controls.set_all_encoder_leds(Palette::off);

	// TODO: friction

	// morph step
	controls.set_encoder_led(1, Palette::grey.blend(Palette::red, params.morph_step));
	auto inc = controls.encoders[1].read();
	params.morph_step += (1.f / 100.f) * inc;
	params.morph_step = std::clamp(params.morph_step, 0.f, 1.f);

	// TODO: bounce

	// seq length
	// not needed here

	// random seeding
	// TODO: there is likely a better way to do this.
	static int random_seed = 1;
	controls.set_encoder_led(6, Palette::from_raw(random_seed));
	inc = controls.encoders[6].read();
	if (inc) {
		// this feels random enough
		random_seed += controls.read_cv() * controls.read_slider();
		std::srand(random_seed);
		params.banks.randomize();
	}

	// quantize
	// chromatic, major, minor, pentatonic?
	// magenta, blue, red, green... ?

	state = controls.alt_button.is_high() ? state : State::MacroIdleInit;
}

void UI::macro_state_alt_scene()
{
	display_output = false;
	controls.set_all_encoder_leds(Palette::off);

	// random
	auto scene = params.pathway.scene_nearest();
	auto temp = params.banks.get_scene_random_amount(scene);
	auto color = Palette::grey.blend(Palette::red, temp);
	if (temp == 0.f)
		color = Palette::green;
	controls.set_encoder_led(6, color);
	auto inc = controls.encoders[6].read();
	temp += (1.f / 100.f) * inc;
	params.banks.set_scene_random_amount(scene, temp);

	state = controls.alt_button.is_high() ? state : State::MacroIdleInit;
}

void UI::global_state_bank()
{
	scene_button_high([&](unsigned chan) { params.banks.sel_bank(chan); });
	controls.set_button_led(params.banks.get_sel_bank(), true);

	State idle_state = State::MacroIdleInit;
	if (params.mode == Params::Mode::Sequencer)
		idle_state = State::SeqIdleInit;

	state = controls.bank_button.is_high() ? state : idle_state;
}

void UI::macro_state_ab()
{
	encoder_display_pathway_size();

	params.pathway.update(params.slider_pos);
	scene_button_display_nearest();

	/* a and b buttons*/
	auto a = controls.a_button.is_high();
	auto b = controls.b_button.is_high();

	if (a && b) {
		params.pathway.clear_scenes();
		return;
	}

	if (a) {
		scene_button_just_went_low([&](unsigned scene) {
			if (!controls.a_button.just_went_high()) {
				params.pathway.insert_scene(scene, true);
				return; // return from lambda
			}

			if (params.pathway.on_a_scene())
				params.pathway.replace_scene(scene);
			else
				params.pathway.insert_scene(scene, false);
		});

		return;
	}

	if (b) {
		// clear recording
		if (controls.play_button.is_high()) {
			recorder.clear();
			return;
		}

		// removing a scene is not nearly as intuitive as inserting them
		// TODO: make it better
		if (params.pathway.on_a_scene()) {
			scene_button_just_went_low([&](unsigned scene) {
				if (scene == params.pathway.scene_nearest())
					params.pathway.remove_scene();
			});
		}
		return;
	}

	state = State::MacroIdleInit;
}
} // namespace Catalyst2
