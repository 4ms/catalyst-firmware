#include "ui.hh"

namespace Catalyst2
{

void UI::update_mode()
{
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
	controls.for_each_scene_butt_released([this](unsigned chan) {
		params.seq.sel_chan(chan);
		if (controls.latch_button.is_high()) {
			if (!params.seq.is_chan_selected())
				params.seq.sel_chan(chan);
		}
	});
	auto alt = controls.alt_button.is_high();

	if (params.seq.is_chan_selected()) {
		controls.for_each_encoder_inc(
			[this, alt](int inc, unsigned scene) { params.banks.IncChan(scene, params.seq.get_sel_chan(), inc, alt); });
	} else {
		controls.for_each_encoder_inc([this, alt](int inc, unsigned scene) {
			; // clear encoder states
		});
	}
	if (params.override_output.has_value()) {

		if (controls.latch_button.just_went_high()) {
			params.banks.CopySceneToClipboard(params.override_output.value());
			params.seq.sel_chan(Model::NumChans);
		} else if (controls.latch_button.is_high()) {
			if (controls.scene_buttons[params.override_output.value()].just_went_high())
				params.banks.PasteToScene(params.override_output.value());
		}
	} else {
		if (controls.latch_button.just_went_high())
			; // necessary for copy paste
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

	if (params.override_output.has_value()) {
		// copy paste
		if (controls.latch_button.just_went_high()) {
			params.banks.CopySceneToClipboard(params.override_output.value());
		} else if (controls.latch_button.is_high()) {
			if (controls.scene_buttons[params.override_output.value()].just_went_high())
				params.banks.PasteToScene(params.override_output.value());
		}

		// encoders blah blah
		controls.for_each_encoder_inc([this, alt](int inc, unsigned chan) {
			controls.for_each_scene_button_high(
				[this, inc, chan, alt](Pathway::SceneId scene) { params.banks.IncChan(scene, chan, inc, alt); });
		});
	} else {
		if (controls.latch_button.just_went_high())
			; // necessary for copy paste

		controls.for_each_encoder_inc([this, alt](int inc, unsigned chan) {
			if (params.banks.Path().on_a_scene())
				params.banks.IncChan(params.banks.Path().scene_nearest(), chan, inc, alt);
			else {
				params.banks.IncChan(params.banks.Path().scene_left(), chan, inc, alt);
				params.banks.IncChan(params.banks.Path().scene_right(), chan, inc, alt);
			}
		});
	}
}

void UI::state_settings()
{
	const auto alt = controls.alt_button.is_high();

	// morph step
	auto inc = controls.encoders[1].read();
	params.morph_step += (1.f / 100.f) * inc;
	params.morph_step = std::clamp(params.morph_step, 0.f, 1.f);

	// bpm
	inc = controls.encoders[2].read();
	intclock.bpm_inc(inc, alt);

	// random amount
	auto scene = params.banks.Path().scene_nearest();
	if (params.override_output.has_value()) {
		scene = params.override_output.value();
	} else {
	}
	auto temp = params.banks.GetRandomAmount(scene);
	inc = controls.encoders[4].read();
	temp += (1.f / 100.f) * inc;
	params.banks.SetRandomAmount(scene, temp);

	// random seeding
	// turning right gets new rando values and turning left turns off random values
	inc = controls.encoders[6].read();
	const auto b = params.banks.GetSelBank();
	if (inc > 0)
		RandomPool::RandomizeBank(b);
	else if (inc < 0)
		RandomPool::ClearBank(b);

	// quantize
	auto &scl = params.current_scale;
	if ((inc = controls.encoders[7].read())) {
		scl += inc;
		scl = std::clamp<int8_t>(scl, 0, Model::Scales.size() - 1);
		params.quantizer.load_scale(Model::Scales[scl].scale);
	}
}

void UI::state_bank()
{
	params.override_output = std::nullopt;

	for (auto i = 0u; i < Model::NumChans; ++i) {
		const auto inc = controls.encoders[i].read();
		// params.banks.adj_chan_type(i, inc);

		if (controls.scene_buttons[i].is_high()) {
			params.banks.SelBank(i);
			params.banks.Path().refresh();
		}
	}
}

void UI::state_seq_ab()
{
	/* a and b buttons*/
	controls.for_each_scene_butt_released([this](unsigned chan) { params.seq.sel_chan(chan); });

	const auto a = controls.a_button.is_high();
	const auto b = controls.b_button.is_high();

	// don't allow adjusting the length of a non existant sequence
	if (!params.seq.is_chan_selected())
		params.seq.sel_chan(0);

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
	params.override_output = std::nullopt;

	/* a and b buttons*/
	const auto a = controls.a_button.is_high();
	const auto b = controls.b_button.is_high();

	if (a && b) {
		params.banks.Path().clear_scenes();
		return;
	}

	if (a) {
		controls.for_each_scene_butt_released([&](unsigned scene) {
			if (!controls.a_button.just_went_high()) {
				params.banks.Path().insert_scene(scene, true);
				return; // return from lambda
			}

			if (params.banks.Path().on_a_scene())
				params.banks.Path().replace_scene(scene);
			else
				params.banks.Path().insert_scene(scene, false);
		});

		return;
	}

	if (!b)
		return;

	// clear recording
	if (controls.play_button.just_went_high()) {
		recorder.stop();
		recorder.clear_recording();
		return;
	}

	// removing a scene is not nearly as intuitive as inserting them
	// TODO: make it better
	if (params.banks.Path().on_a_scene()) {
		controls.for_each_scene_butt_released([&](unsigned scene) {
			if (scene == params.banks.Path().scene_nearest())
				params.banks.Path().remove_scene();
		});
	}
	return;
}

void UI::paint_leds(const Model::OutputBuffer &outs)
{
	if (!leds_ready_flag)
		return;
	leds_ready_flag = false;

	controls.set_all_encoder_leds(Palette::off);
	controls.clear_button_leds();

	switch (state) {
		case State::Main:
			if (params.mode == Params::Mode::Macro) {
				if (params.override_output.has_value()) {
					encoder_display_scene(params.override_output.value());
					controls.for_each_scene_button_high(
						[this](Pathway::SceneId scene) { controls.set_button_led(scene, true); });
				} else {
					encoder_display_output(outs);
					if (recorder.is_recordering())
						scene_button_display_recording();
					else {
						const auto l = params.banks.Path().scene_left();
						const auto r = params.banks.Path().scene_right();
						if (l == r)
							controls.set_button_led(l, true);
						else {
							controls.set_button_led(l, 1.f - params.pos);
							controls.set_button_led(r, params.pos);
						}
					}
				}

			} else if (params.mode == Params::Mode::Sequencer) {
				if (params.seq.is_chan_selected()) {
					encoder_display_sequence();
					controls.set_button_led(params.seq.get_sel_chan(), true);
				} else
					encoder_display_output(outs);
			}
			break;
		case State::AB: {
			if (params.mode == Params::Mode::Macro) {
				encoder_display_pathway_size();
				scene_button_display_nearest();
			} else if (params.mode == Params::Mode::Sequencer) {
				encoder_display_sequence_length();
				controls.set_button_led(params.seq.get_sel_chan(), true);
			}
			break;
		}
		case State::Settings: {
			// scene button
			const auto scene = params.override_output.has_value() ? params.override_output.value() :
							   params.banks.Path().on_a_scene()	  ? params.banks.Path().scene_nearest() :
																	Model::NumScenes;

			if (scene < Model::NumScenes) {
				controls.set_button_led(scene, true);

				// random amount
				const auto temp = params.banks.GetRandomAmount(scene);
				auto c = Palette::grey.blend(Palette::red, temp);
				if (temp == 0.f)
					c = Palette::green;
				controls.set_encoder_led(4, c);
			}

			// morph step
			auto c = Palette::grey.blend(Palette::red, params.morph_step);
			if (params.morph_step == 0.f)
				c = Palette::green;
			controls.set_encoder_led(1, c);

			// bpm
			const auto flipper = params.seq.get_clock();
			controls.set_encoder_led(2, flipper ? Palette::off : Palette::orange);

			// random seed
			controls.set_encoder_led(6, Palette::from_raw(RandomPool::GetSeed(params.banks.GetSelBank())));

			// quantize
			controls.set_encoder_led(7, Model::Scales[params.current_scale].color);
			break;
		}
		case State::Bank: {
			for (auto i = 0u; i < Model::NumChans; i++) {
				auto c = Color{2, 0, 0};
				if (params.banks.IsChanTypeGate(i))
					c = Color{0, 0, 2};
				else if (params.banks.IsChanQuantized(i))
					c = Color{0, 2, 0};
				controls.set_encoder_led(i, c);
			}
			controls.set_button_led(params.banks.GetSelBank(), true);

			break;
		}
	}
}
} // namespace Catalyst2
