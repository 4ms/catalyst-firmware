#pragma once
#include "conf/model.hh"
#include "params.hh"
#include "util/countzip.hh"
#include "util/math.hh"
#include <algorithm>

namespace MathTools
{
// if slope == 0 actual slope == 1
// if slope is >= (max - min) actual slope == inf
// if slope is negative than actual slope is less than 1
constexpr float slope_adj(float in, float slope, float min, float max)
{
	const auto range = max - min;
	const auto b = range / 2.f;

	if (slope >= range) {
		return in < b ? min : max;
	}

	const auto m = range / (range - slope);
	const auto x = in - b;
	const auto y = (m * x) + b;
	return constrain(y, min, max);
}
} // namespace MathTools

namespace Catalyst2
{

class Trigger {
	static constexpr uint8_t trig_length = 20;
	uint8_t trig_time = 0;
	bool is_trigged = false;

public:
	void trig(uint8_t time_now)
	{
		if (is_trigged)
			return;

		trig_time = time_now;
		is_trigged = true;
	}
	bool get(uint8_t time_now)
	{
		if (!is_trigged)
			return false;
		uint8_t elapsed = time_now - trig_time;
		if (elapsed >= trig_length) {
			is_trigged = false;
			return false;
		}

		return true;
	}
};

class Tick {
	uint8_t c = 0;

public:
	void Update()
	{
		c++; // haha
	}
	uint8_t get()
	{
		return c;
	}
};

class MacroSeq {
	Params &params;
	Tick tick;
	std::array<Trigger, Model::NumChans> trigger;

public:
	MacroSeq(Params &params)
		: params{params}
	{}

	auto update()
	{
		tick.Update();

		Model::OutputBuffer buf;

		params.pathway.update(params.pos);

		if (params.mode == Params::Mode::Macro)
			macro(buf);
		else
			seq(buf);

		for (auto &out : buf) {
			out = params.quantizer.process(out);
		}

		return buf;
	}

private:
	void macro(auto &in)
	{
		static auto do_trigs = false;
		const auto time_now = tick.get();

		if (params.override_output.has_value()) {
			for (auto [chan, out] : countzip(in)) {
				if (params.banks.is_chan_type_gate(chan)) {
					auto is_primed = params.banks.get_chan(params.override_output.value(), chan);
					if (do_trigs && is_primed == ChannelValue::GateSetFlag)
						trigger[chan].trig(time_now);
					out = trigger[chan].get(time_now) ? ChannelValue::GateHigh : ChannelValue::from_volts(0.f);
				} else {
					out = params.banks.get_chan(params.override_output.value(), chan);
				}
			}
			do_trigs = false;
			return;
		}

		const auto left = params.pathway.scene_left();
		const auto right = params.pathway.scene_right();

		auto phase = params.pos / params.pathway.get_scene_width();
		phase -= static_cast<unsigned>(phase);
		phase = MathTools::slope_adj(phase, params.morph_step, 0.f, 1.f);
		params.pos = phase;

		static Pathway::SceneId last_scene_on = Model::NumScenes;
		const Pathway::SceneId current_scene =
			params.pathway.on_a_scene() ? params.pathway.scene_nearest() : Model::NumScenes;
		do_trigs = false;
		if (current_scene != last_scene_on) {
			last_scene_on = current_scene;
			if (current_scene < Model::NumScenes)
				do_trigs = true;
		}

		for (auto [chan, out] : countzip(in)) {
			if (params.banks.is_chan_type_gate(chan)) {
				auto is_primed = ChannelValue::from_volts(0.f);
				if (current_scene < Model::NumScenes)
					is_primed = params.banks.get_chan(current_scene, chan);
				if (do_trigs && is_primed == ChannelValue::GateSetFlag) {
					trigger[chan].trig(time_now);
				}

				out = trigger[chan].get(time_now) ? ChannelValue::GateHigh : is_primed;
			} else {
				const auto a = params.banks.get_chan(left, chan);
				const auto b = params.banks.get_chan(right, chan);
				out = MathTools::interpolate(a, b, phase);
			}
		}

		do_trigs = true;
	}
	void seq(Model::OutputBuffer &in)
	{
		static auto prev_ = false;
		auto cur = params.seq.get_clock();
		bool do_trigs = false;
		if (prev_ != cur) {
			prev_ = cur;
			do_trigs = true;
		}
		const auto time_now = tick.get();

		for (auto [chan, o] : countzip(in)) {
			const auto step = params.seq.get_step(chan);
			if (params.banks.is_chan_type_gate(chan)) {
				auto is_primed = params.banks.get_chan(step, chan);
				if (do_trigs && is_primed == ChannelValue::GateSetFlag)
					trigger[chan].trig(time_now);
				o = trigger[chan].get(time_now) ? ChannelValue::GateHigh : is_primed;
			} else {
				o = params.banks.get_chan(step, chan);
			}
		}

		Model::OutputBuffer temp;

		auto phase = params.pos / (1.f / (Model::NumChans - 1));
		const auto middle = static_cast<unsigned>(phase) & (Model::NumChans - 1);
		std::rotate(in.begin(), &in[Model::NumChans - middle - 1], in.end());
		phase -= static_cast<unsigned>(phase);
		phase = MathTools::slope_adj(phase, params.morph_step, 0.f, 1.f);

		for (auto i = 0u; i < Model::NumChans; i++) {
			temp[i] = MathTools::interpolate(in[(i + 1) & (Model::NumChans - 1)], in[i], phase);
		}

		in = temp;
	}
};

} // namespace Catalyst2
