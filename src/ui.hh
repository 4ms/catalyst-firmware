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
		Macro,
		MacroAB,
		Settings,
		Bank,
		Seq,
		Reset,
	};
	State state = State::Reset;
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
		update_mode();
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
	void update_slider_and_cv()
	{
		auto slider = controls.read_slider();
		auto cv = controls.read_cv();
		auto rec = recorder.update(slider + cv);
		params.pos = rec / 4096.f;
		params.slider_pos = slider / 4096.f;
	}

	void update_mode();

	void state_macro();
	void state_macro_ab();
	void state_settings();
	void state_bank();
	void state_seq();

	void seq_update_step()
	{
		auto step = false;

		if (!controls.trig_jack_sense.is_high()) {
			step = controls.trig_jack.just_went_high();
		} else {
			intclock.update();
			step = intclock.step();
		}

		if (step)
			params.seq.step();
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
		controls.set_button_led(chan, true);

		encoder_display_count(Palette::red, length);
	}

	void encoder_display_sequence()
	{
		display_output = false;
		auto chan = params.seq.get_sel_chan();
		auto cur_step = params.seq.get_step(chan);
		auto length = params.seq.get_length(chan);
		controls.set_button_led(chan, true);

		for (auto i = 0u; i < length; i++) {
			if (i == cur_step) {
				controls.set_encoder_led(i, Palette::magenta);
			} else {
				auto level = params.banks.get_chan(i, chan);
				controls.set_encoder_led(i, encoder_blend(level));
			}
		}
		for (auto i = 0u; i < Model::NumChans - length; i++)
			controls.set_encoder_led(length + i, Palette::off);
	}

	// encoder display helper funcs
	void encoder_display_count(Color c, unsigned count)
	{
		if (count > Model::NumChans)
			return;

		for (auto i = 0u; i < count; i++)
			controls.set_encoder_led(i, c);

		for (auto i = 0u; i < Model::NumChans - count; i++)
			controls.set_encoder_led(count + i, Palette::off);
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
