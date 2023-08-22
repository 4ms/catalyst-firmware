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
	bool fine_inc = false;

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

		update_slider();
		update_cv();
		update_mode();

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

	void update_slider()
	{
		auto slider = controls.read_slider();
		params.slider_pos = slider / 4096.f;
	}

	void update_cv()
	{
		auto cv = controls.read_cv();
		params.cv_offset = cv / 4096.f;
	}

	void macro_state_idle();
	void macro_state_ab();
	void macro_state_alt();
	void macro_state_bank();

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
			phase /= zero_v;
			return Palette::red.blend(Palette::yellow, phase);
		} else if (phase < five_v) {
			phase -= zero_v;
			phase /= zero_v;
			return Palette::yellow.blend(Palette::green, phase);
		} else {
			phase -= five_v;
			phase /= zero_v;
			return Palette::green.blend(Palette::blue, phase);
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
