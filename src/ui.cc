#include "ui.hh"
#include "conf/quantizer_scales.hh"

namespace Catalyst2
{

void UI::update_mode()
{

	// common to all states
	controls.clear_button_leds();
	display_output = true;

	switch (state) {
		case State::Settings:
			state_settings();

			if (controls.alt_button.is_high() || controls.b_button.is_high())
				return;
			break;

		case State::AB:
			if (params.mode == Params::Mode::Macro)
				state_macro_ab();
			else
				state_seq_ab();

			if (controls.a_button.is_high())
				return;
			if (controls.b_button.is_high()) {
				if (controls.alt_button.is_high())
					state = State::Settings;
				return;
			}
			break;

		case State::Bank:
			state_bank();

			if (controls.bank_button.is_high())
				return;
			break;

		case State::Main:
			if (params.mode == Params::Mode::Macro)
				state_macro();
			else
				state_seq();

			if (controls.alt_button.is_high() && controls.b_button.is_high())
				state = State::Settings;
			else if (controls.a_button.is_high() || controls.b_button.is_high()) {
				state = State::AB;
				controls.scene_buttons_clear_events();
			} else if (controls.bank_button.is_high())
				state = State::Bank;
			return;
	}
	state = State::Main;
}

void UI::state_seq()
{
	on_scene_button_release([this](unsigned chan) { params.seq.sel_chan(chan); });
	auto alt = controls.alt_button.is_high();

	if (params.seq.is_chan_selected()) {
		encoder_display_sequence();
		on_encoder_inc([this, alt](int inc, unsigned scene) {
			params.banks.adj_chan(scene, params.seq.get_sel_chan(), inc, alt);
		});
	} else {
		;
	}
}

void UI::state_macro()
{
	const auto alt = controls.alt_button.is_high();

	if (controls.play_button.just_went_high()) {
		if (alt)
			recorder.cue_recording();
		if (controls.trig_jack_sense.is_high())
			recorder.reset();
	}

	if (recorder.is_recordering()) {
		scene_button_display_recording();
		return;
	}

	// display the last one pressed.
	Pathway::SceneId scene_to_display = 0xff;
	auto age = 0xffffffffu;

	if (on_scene_button_high([this, &scene_to_display, &age](Pathway::SceneId scene) {
			controls.set_button_led(scene, true);

			if (controls.scene_buttons[scene].time_high < age) {
				age = controls.scene_buttons[scene].time_high;
				scene_to_display = scene;
			}
		}))
	{
		// do this once regardless if any amount of buttons are pressed.
		// is this too redundant?
		on_encoder_inc([this, alt](int inc, unsigned chan) {
			on_scene_button_high(
				[this, inc, chan, alt](Pathway::SceneId scene) { params.banks.adj_chan(scene, chan, inc, alt); });
		});

		// TODO: somehow this scene needs to be sent out the output
		encoder_display_scene(scene_to_display);

		return;
	}

	// do this if no scene buttons were pressed.
	// display the current scene
	scene_button_display_nearest();

	on_encoder_inc([this, alt](int inc, unsigned chan) {
		get_scene_context(
			[this, inc, chan, alt](Pathway::SceneId scene) { params.banks.adj_chan(scene, chan, inc, alt); });
	});
}

void UI::state_settings()
{
	display_output = false;
	controls.set_all_encoder_leds(Palette::off);

	// morph step
	controls.set_encoder_led(1, Palette::grey.blend(Palette::red, params.morph_step));
	auto inc = controls.encoders[1].read();
	params.morph_step += (1.f / 100.f) * inc;
	params.morph_step = std::clamp(params.morph_step, 0.f, 1.f);

	// TODO: bounce?

	// random amount
	auto scene = params.pathway.scene_nearest();
	auto temp = params.banks.get_scene_random_amount(scene);
	auto color = Palette::grey.blend(Palette::red, temp);
	if (temp == 0.f)
		color = Palette::green;
	controls.set_encoder_led(4, color);
	inc = controls.encoders[4].read();
	temp += (1.f / 100.f) * inc;
	params.banks.set_scene_random_amount(scene, temp);

	// random seeding
	// turning right gets new rando values and turning left turns off random values
	controls.set_encoder_led(6, Palette::from_raw(params.banks.get_random_seed()));
	inc = controls.encoders[6].read();
	if (inc > 0)
		params.banks.randomize();
	else if (inc < 0)
		params.banks.clear_random();

	// quantize
	static int8_t cur_scale = 0;
	controls.set_encoder_led(7, Model::Scales[cur_scale].color);
	if ((inc = controls.encoders[7].read())) {
		cur_scale += inc;
		cur_scale = std::clamp<int8_t>(cur_scale, 0, Model::Scales.size() - 1);
		params.quantizer.load_scale(Model::Scales[cur_scale].scale);
	}
}

void UI::state_bank()
{
	display_output = false;
	// display which banks are edited vs which ones arent on the encoders.
	for (auto x = 0u; x < Model::NumBanks; ++x) {
		auto edt = params.banks.is_edited(x);
		Color c = Palette::off;
		if (edt)
			c = Palette::tangerine;
		controls.set_encoder_led(x, c);

		auto inc = controls.encoders[x].read();

		if (inc < 0)
			params.banks.reset_bank(x);
	}

	on_scene_button_high([&](unsigned chan) { params.banks.sel_bank(chan); });
	controls.set_button_led(params.banks.get_sel_bank(), true);
}

void UI::state_seq_ab()
{
	encoder_display_sequence_length();

	/* a and b buttons*/
	const auto a = controls.a_button.is_high();
	const auto b = controls.b_button.is_high();
	const auto chan = params.seq.get_sel_chan();

	if (a && b) {
		params.seq.reset_length(chan);
		params.seq.reset_start_offset(chan);
		return;
	}

	auto inc = controls.encoders[5].read();

	if (!inc)
		return;

	if (a) {
		params.seq.adj_start_offset(chan, inc);
		return;
	}

	if (b)
		params.seq.adj_length(chan, inc);
}

void UI::state_macro_ab()
{
	encoder_display_pathway_size();

	scene_button_display_nearest();

	/* a and b buttons*/
	const auto a = controls.a_button.is_high();
	const auto b = controls.b_button.is_high();

	if (a && b) {
		params.pathway.clear_scenes();
		return;
	}

	if (a) {
		on_scene_button_release([&](unsigned scene) {
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

	if (!b)
		return;

	// clear recording
	if (controls.play_button.just_went_high()) {
		recorder.stop();
		return;
	}

	// removing a scene is not nearly as intuitive as inserting them
	// TODO: make it better
	if (params.pathway.on_a_scene()) {
		on_scene_button_release([&](unsigned scene) {
			if (scene == params.pathway.scene_nearest())
				params.pathway.remove_scene();
		});
	}
	return;
}
} // namespace Catalyst2
