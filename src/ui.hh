#pragma once
#include "conf/model.hh"
#include "conf/palette.hh"
//#include "conf/quantizer_scales.hh"
#include "controls.hh"
#include "intclock.hh"
#include "outputs.hh"
#include "params.hh"
#include "recorder.hh"
#include "util/countzip.hh"

namespace Catalyst2
{

class UI {
	enum class State {
		Main,
		Settings,
		Bank,
		AB,
	};
	State state;
	Controls controls;
	Params &params;
	InternalClock<Board::cv_stream_hz> intclock;

	// should the recorder be in params?
	Recorder recorder;
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
		HAL_Delay(2);
		std::srand(controls.read_slider() + controls.read_cv());
		params.banks.clear_random();
	}

	void update()
	{
		controls.update();
		update_slider_and_cv();
		update_switch();
		update_trig_jack();
		update_reset_jack();
		update_mode();
		update_state();
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
	void update_mode();
	void state_macro();
	void state_macro_ab();
	void state_settings();
	void state_bank();
	void state_seq();
	void state_seq_ab();

	void update_state()
	{
		switch (state) {
			case State::Settings:

				break;

			case State::Bank:

				break;

			case State::AB:

				break;

			case State::Main:
				break;
		}
	}

	void update_slider_and_cv()
	{
		auto slider = controls.read_slider();
		auto cv = controls.read_cv();
		auto rec = recorder.update(slider + cv);
		params.pos = rec / 4096.f;
		params.slider_pos = slider / 4096.f;
	}

	void update_switch()
	{
		if (controls.mode_switch.just_went_high()) {
			params.mode = Params::Mode::Sequencer;
			params.seq.reset();
		} else if (controls.mode_switch.just_went_low()) {
			params.mode = Params::Mode::Macro;
		}
	}

	void update_trig_jack()
	{

		const auto macro = params.mode == Params::Mode::Macro;

		if (controls.trig_jack_sense.is_high()) {
			if (macro)
				return;

			intclock.update();

			if (intclock.step())
				params.seq.step();

			return;
		}

		const bool edge = controls.trig_jack.just_went_high();

		if (!edge)
			return;

		if (!macro) {
			params.seq.step();
			return;
		}

		recorder.reset();
	}

	void update_reset_jack()
	{
		const auto edge = controls.reset_jack.just_went_high();

		if (!edge)
			return;

		if (params.mode == Params::Mode::Sequencer) {
			params.seq.reset();
			return;
		}

		// macro mode
		// what should reset do???
		// recorder.cue_recording();
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

	void on_scene_button_release(auto f)
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
	bool on_scene_button_high(auto f)
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

	void on_encoder_inc(auto f)
	{
		for (auto [i, enc] : countzip(controls.encoders)) {
			auto inc = enc.read();
			if (inc)
				f(inc, i);
		}
	}

	void scene_button_display_recording()
	{
		auto led = static_cast<unsigned>(recorder.capacity_filled() * 8u);
		auto level = recorder.size() & 0x10;

		controls.set_button_led(led, level);
	}

	void scene_button_display_nearest()
	{
		get_scene_context([&](Pathway::SceneId scene) { controls.set_button_led(scene, true); });
	}

	// encoder display funcs
	void encoder_display_pathway_size()
	{
		display_output = false;
		auto r = params.pathway.size();
		auto phase = 1.f / (params.pathway.MaxPoints / static_cast<float>(r));

		while (r > 8)
			r -= 8;

		encoder_display_count(Palette::green.blend(Palette::red, phase), r);
	}

	void encoder_display_scene(Pathway::SceneId scene)
	{
		display_output = false;
		for (auto i = 0u; i < Model::NumChans; i++) {
			auto temp = params.banks.get_chan(scene, i);
			controls.set_encoder_led(i, encoder_blend(temp));
		}
	}

	void encoder_display_sequence_length()
	{
		display_output = false;
		auto chan = params.seq.get_sel_chan();
		auto length = params.seq.get_length(chan);
		auto offset = params.seq.get_start_offset(chan);

		encoder_display_count(Palette::red, length, offset);
		controls.set_encoder_led(offset, Palette::grey);
	}

	void encoder_display_sequence()
	{
		display_output = false;
		const auto chan = params.seq.get_sel_chan();
		const auto cur_step = params.seq.get_step(chan);
		const auto length = params.seq.get_length(chan);
		const auto offset = params.seq.get_start_offset(chan);

		for (auto x = offset; x < length + offset; x++) {
			const auto i = x & 7;
			if (i == cur_step) {
				controls.set_encoder_led(i, Palette::magenta);
			} else {
				const auto level = params.banks.get_chan(i, chan);
				controls.set_encoder_led(i, encoder_blend(level));
			}
		}
		for (auto x = offset; x < Model::NumChans - length + offset; x++)
			controls.set_encoder_led((length + x) & 7, Palette::off);
	}

	// encoder display helper funcs
	void encoder_display_count(Color c, unsigned count, unsigned offset = 0)
	{
		if (count > Model::NumChans)
			return;

		for (auto i = 0u; i < count; i++)
			controls.set_encoder_led((i + offset) & 7, c);

		for (auto i = 0u; i < Model::NumChans - count; i++)
			controls.set_encoder_led((count + i + offset) & 7, Palette::off);
	}

	Color encoder_blend(uint16_t level)
	{
		auto phase = static_cast<uint8_t>(level >> 8);
		return Palette::red.blend(Palette::yellow, phase);
	}

	Color encoder_blend_old(uint16_t level)
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

	mdrivlib::Timekeeper encoder_led_update_task;
};

} // namespace Catalyst2
