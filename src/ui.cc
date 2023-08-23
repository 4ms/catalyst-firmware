#include "ui.hh"

namespace Catalyst2
{

void UI::update_mode()
{
	update_mode_switch();

	if (params.mode == Params::Mode::Macro) {

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
				// we can initialize states here.
				if (controls.alt_button.just_went_high()) {
					// clear falling edges.
					controls.play_button.just_went_high();
					controls.play_button.just_went_low();
					controls.bank_button.just_went_low();
					controls.b_button.just_went_low();
					scene_button_just_went_low([](Pathway::SceneId garb) {});
					state = State::MacroAlt;
				} else if (controls.a_button.just_went_high() || controls.b_button.just_went_high()) {
					state = State::MacroAB;
					// clear the falling edge states
					scene_button_just_went_low([](Pathway::SceneId garb) {});
				} else if (controls.bank_button.just_went_high()) {
					state = State::MacroBank;
				}

				macro_state_idle();
				break;
		}

	} else {
	}

	// sequencer mode
}

void UI::macro_state_idle()
{
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
			scene_button_high([&](Pathway::SceneId scene) { params.banks.adj_chan(scene, chan, inc, fine_inc); });
		});

		// TODO: somehow this scene needs to be sent out the output
		encoder_display_scene(scene_to_display);
	} else {
		// do this if no scene buttons were pressed.
		display_output = true;

		// display the current scene based on slider pos
		get_scene_context([&](Pathway::SceneId scene) { controls.set_button_led(scene, true); });

		get_encoder([&](int inc, unsigned chan) {
			get_scene_context([&](Pathway::SceneId scene) { params.banks.adj_chan(scene, chan, inc, fine_inc); });
		});
	}
}

void UI::macro_state_alt()
{
	// holding alt and then pressing B should get into the menu where morph, friction etc are adjusted
	enum class AltState {
		GLOBAL,
		SCENE,
		IDLE,
	};
	static AltState altstate = AltState::IDLE;
	static int random_seed = 1;

	if (controls.b_button.just_went_low())
		altstate = AltState::GLOBAL;
	else if (controls.bank_button.just_went_low())
		altstate = AltState::SCENE;

	switch (altstate) {
		case AltState::GLOBAL: {
			display_output = false;
			controls.set_all_encoder_leds(Palette::off);

			// morph step
			controls.set_encoder_led(1, Palette::grey.blend(Palette::red, params.morph_step));
			auto inc = controls.encoders[1].read();
			params.morph_step += (1.f / 100.f) * inc;
			params.morph_step = std::clamp(params.morph_step, 0.f, 1.f);

			// random seeding
			controls.set_encoder_led(6, Palette::from_raw(random_seed));
			inc = controls.encoders[6].read();
			if (inc) {
				// this feels random enough
				random_seed = controls.read_cv();
				std::srand(random_seed);
				params.banks.randomize();
			}

			break;
		}
		case AltState::SCENE: {
			display_output = false;
			controls.set_all_encoder_leds(Palette::off);
			auto scene = params.pathway.scene_nearest();
			auto temp = params.banks.get_scene_random_amount(scene);
			auto color = Palette::grey.blend(Palette::red, temp);
			if (temp == 0.f)
				color = Palette::green;
			controls.set_encoder_led(6, color);
			auto inc = controls.encoders[6].read();
			temp += (1.f / 100.f) * inc;
			params.banks.set_scene_random_amount(scene, temp);
			break;
		}
		case AltState::IDLE: {
			// just turn on fine tuning. we can also check if it's time to record a phrase
			fine_inc = true;
			macro_state_idle();

			if (controls.play_button.just_went_high()) {
				recorder.record();
			}
			if (controls.play_button.just_went_low()) {
				recorder.stop();
				recorder.play();
			}
			break;
		}
	}

	if (!controls.alt_button.is_high()) {
		state = State::MacroIdle;
		fine_inc = false;
		altstate = AltState::IDLE;
	}
}

void UI::macro_state_bank()
{
	scene_button_high([&](unsigned chan) { params.banks.sel_bank(chan); });
	controls.set_button_led(params.banks.get_sel_bank(), true);

	if (!controls.bank_button.is_high()) {
		state = State::MacroIdle;
	}
}

void UI::macro_state_ab()
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
} // namespace Catalyst2
