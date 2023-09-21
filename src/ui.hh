#pragma once
#include "conf/model.hh"
#include "conf/palette.hh"
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
	bool encoder_leds_ready_flag = false;

public:
	UI(Params &params)
		: params{params}
	{
		encoder_led_update_task.init(Board::encoder_led_task, [&]() {
			controls.write_to_encoder_leds();
			encoder_leds_ready_flag = true;
		});
		muxio_update_task.init(Board::muxio_conf, [&]() { controls.update_muxio(); });
	}

	void start()
	{
		encoder_led_update_task.start();
		muxio_update_task.start();
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
		update_output_override();
		update_mode();
	}

	void set_outputs(const Model::OutputBuffer &outs)
	{
		outputs.write(outs);
		if (display_output) {
			if (encoder_leds_ready()) {
				for (auto [chan, val] : countzip(outs)) {
					Color c;
					if (params.banks.is_chan_type_gate(chan))
						c = encoder_gate_blend(val);
					else
						c = encoder_cv_blend(val);
					controls.set_encoder_led(chan, c);
				}
			}
			auto l = params.pathway.scene_left();
			auto r = params.pathway.scene_right();
			if (l == r)
				controls.set_button_led(l, 1.f);
			else {
				controls.set_button_led(l, 1.f - params.pos);
				controls.set_button_led(r, params.pos);
			}
		}

		if (state == State::Main && !params.override_output.has_value()) {
			// yea/
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

	void update_output_override()
	{
		params.override_output = controls.youngest_scene_button();
	}

	void update_slider_and_cv()
	{
		auto temp = controls.read_slider();
		temp += controls.read_cv();
		params.pos = recorder.update(temp) / 4096.f;
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
		// edge?
		if (!controls.reset_jack.just_went_high())
			return;

		if (params.mode == Params::Mode::Sequencer) {
			params.seq.reset();
			return;
		}

		// macro mode
		// what should reset do???
		// recorder.cue_recording();
	}

	void scene_button_display_recording()
	{
		auto led = static_cast<unsigned>(recorder.capacity_filled() * 8u);
		auto level = recorder.size() & 0x10;

		controls.set_button_led(led, level);
	}

	void scene_button_display_nearest()
	{
		if (params.pathway.on_a_scene())
			controls.set_button_led(params.pathway.scene_nearest(), 1.f);
	}

	// encoder display funcs
	void encoder_display_pathway_size()
	{
		display_output = false;
		if (!encoder_leds_ready())
			return;
		auto r = params.pathway.size();
		auto phase = 1.f / (params.pathway.MaxPoints / static_cast<float>(r));

		while (r > 8)
			r -= 8;

		encoder_display_count(Palette::green.blend(Palette::red, phase), r);
	}

	void encoder_display_scene(Pathway::SceneId scene)
	{
		display_output = false;
		if (!encoder_leds_ready())
			return;
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			auto temp = params.banks.get_chan(scene, chan);
			Color c;
			if (params.banks.is_chan_type_gate(chan))
				c = encoder_gate_blend(temp);
			else
				c = encoder_cv_blend(temp);
			controls.set_encoder_led(chan, c);
		}
	}

	void encoder_display_sequence_length()
	{
		display_output = false;
		if (!encoder_leds_ready())
			return;
		auto chan = params.seq.get_sel_chan();
		auto length = params.seq.get_length(chan);
		auto offset = params.seq.get_start_offset(chan);

		encoder_display_count(Palette::red, length, offset);
		controls.set_encoder_led(offset, Palette::grey);
	}

	void encoder_display_sequence()
	{
		display_output = false;
		if (!encoder_leds_ready())
			return;

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
				controls.set_encoder_led(i, encoder_cv_blend(level));
			}
		}
		for (auto x = offset; x < Model::NumChans - length + offset; x++)
			controls.set_encoder_led((length + x) & 7, Palette::off);
	}

	// encoder display helper funcs
	bool encoder_leds_ready()
	{
		if (!encoder_leds_ready_flag)
			return false;

		encoder_leds_ready_flag = false;
		return true;
	}

	void encoder_display_count(Color c, unsigned count, unsigned offset = 0)
	{
		if (count > Model::NumChans)
			return;

		for (auto i = 0u; i < count; i++)
			controls.set_encoder_led((i + offset) & 7, c);

		for (auto i = 0u; i < Model::NumChans - count; i++)
			controls.set_encoder_led((count + i + offset) & 7, Palette::off);
	}

	Color encoder_cv_blend(uint16_t level)
	{
		constexpr auto neg = ChannelValue::from_volts(0.f);

		auto temp = level - neg;

		auto c = Palette::blue;

		if (temp < 0) {
			temp *= -2;
			c = Palette::red;
		}

		const auto phase = (temp / (neg * 2.f));

		return Palette::off.blend(c, phase);
	}

	Color encoder_gate_blend(uint16_t level)
	{
		return level == ChannelValue::from_volts(5.0) ? Palette::green : Color{0, 0, 2};
	}

	mdrivlib::Timekeeper encoder_led_update_task;
	mdrivlib::Timekeeper muxio_update_task;
};

} // namespace Catalyst2
