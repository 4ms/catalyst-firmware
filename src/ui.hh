#pragma once
#include "conf/model.hh"
#include "conf/palette.hh"
#include "conf/quantizer_scales.hh"
#include "controls.hh"
#include "intclock.hh"
#include "outputs.hh"
#include "params.hh"
#include "randompool.hh"
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
	Recorder recorder;
	Outputs outputs;
	bool leds_ready_flag = false;

public:
	UI(Params &params)
		: params{params}
	{
		encoder_led_update_task.init(Board::encoder_led_task, [&]() {
			controls.write_to_encoder_leds();
			leds_ready_flag = true;
		});
		muxio_update_task.init(Board::muxio_conf, [&]() { controls.update_muxio(); });
	}

	void start()
	{
		encoder_led_update_task.start();
		muxio_update_task.start();
		controls.start();
		HAL_Delay(2);
		RandomPool::Init(controls.read_slider() + controls.read_cv());
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
		paint_leds(outs);
	}

private:
	void update_mode();
	void state_macro();
	void state_macro_ab();
	void state_settings();
	void state_bank();
	void state_seq();
	void state_seq_ab();
	void paint_leds(const Model::OutputBuffer &outs);

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
			// if (macro)
			//	return;

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
		auto level = (recorder.size() & 0x10) > 0;

		controls.set_button_led(led, level);
	}

	void scene_button_display_nearest()
	{
		auto &p = params.banks.Path();
		if (p.on_a_scene())
			controls.set_button_led(p.scene_nearest(), 1.f);
	}

	// encoder display funcs
	void encoder_display_pathway_size()
	{
		auto &p = params.banks.Path();

		auto r = p.size();
		auto phase = 1.f / (p.MaxPoints / static_cast<float>(r));

		while (r > 8)
			r -= 8;

		encoder_display_count(Palette::green.blend(Palette::red, phase), r);
	}

	void encoder_display_output(const Model::OutputBuffer &buf)
	{
		for (auto [chan, val] : countzip(buf)) {
			Color c = encoder_blend(val, params.banks.IsChanTypeGate(chan));
			controls.set_encoder_led(chan, c);
		}
	}

	void encoder_display_scene(Pathway::SceneId scene)
	{
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			auto temp = params.banks.GetChannel(scene, chan);
			Color c = encoder_blend(temp, params.banks.IsChanTypeGate(chan));
			controls.set_encoder_led(chan, c);
		}
	}

	void encoder_display_sequence_length()
	{
		auto chan = params.seq.get_sel_chan();
		auto length = params.seq.get_length(chan);
		auto offset = params.seq.get_start_offset(chan);

		encoder_display_count(Palette::red, length, offset);
		controls.set_encoder_led(offset, Palette::grey);
	}

	void encoder_display_sequence()
	{
		const auto chan = params.seq.get_sel_chan();
		const auto cur_step = params.seq.get_step(chan);
		const auto length = params.seq.get_length(chan);
		const auto offset = params.seq.get_start_offset(chan);

		for (auto x = offset; x < length + offset; x++) {
			const auto i = x & 7;
			if (i == cur_step) {
				controls.set_encoder_led(i, Palette::magenta);
			} else {
				const auto level = params.banks.GetChannel(i, chan);
				Color c = encoder_blend(level, params.banks.IsChanTypeGate(chan));
				controls.set_encoder_led(i, c);
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

	Color encoder_blend(uint16_t level, bool chan_type_gate)
	{
		if (chan_type_gate)
			return encoder_gate_blend(level);
		else
			return encoder_cv_blend(level);
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
		if (level == ChannelValue::GateSetFlag)
			return Color{0, 0, 2};

		if (level == ChannelValue::GateHigh)
			return Palette::green;

		return Palette::off;
	}

	mdrivlib::Timekeeper encoder_led_update_task;
	mdrivlib::Timekeeper muxio_update_task;
};

} // namespace Catalyst2
