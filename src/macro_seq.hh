#pragma once
#include "conf/model.hh"
#include "params.hh"
#include "trigger.hh"
#include "util/countzip.hh"
#include "util/math.hh"
#include <algorithm>

namespace MathTools
{
// if slope == 0 actual slope == 1
// if slope is >= (max - min) actual slope == inf
// if slope is negative then actual slope is less than 1
constexpr float slope_adj(float in, float slope, float min, float max) {
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

class MacroSeq {
	Params &params;
	std::array<Trigger, Model::NumChans> trigger;

public:
	MacroSeq(Params &params)
		: params{params} {
	}

	auto Update() {
		if (params.mode == Params::Mode::Macro)
			return Macro(params.macro);
		else
			return Seq(params.sequencer);
	}

private:
	Model::OutputBuffer Macro(MacroMode::Interface &p) {
		static auto do_trigs = false;

		Model::OutputBuffer buf;

		const auto time_now = p.shared.internalclock.TimeNow();

		if (p.override_output.has_value()) {
			for (auto [chan, out] : countzip(buf)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					auto is_primed = p.bank.GetChannel(p.override_output.value(), chan);
					if (do_trigs && is_primed >= ChannelValue::GateSetThreshold)
						trigger[chan].Trig(time_now);
					out = trigger[chan].Read(time_now) ? ChannelValue::GateHigh : ChannelValue::from_volts(0.f);
				} else {
					out = p.bank.GetChannel(p.override_output.value(), chan);
				}
			}
			do_trigs = false;
		} else {
			const auto left = p.pathway.SceneLeft();
			const auto right = p.pathway.SceneRight();

			auto phase = p.shared.GetPos() / p.pathway.GetSceneWidth();
			phase -= static_cast<unsigned>(phase);
			p.shared.SetPos(phase); // TODO: is it weird that this is the only time the app changes something in params?

			static Pathway::SceneId last_scene_on = Model::NumScenes;
			const Pathway::SceneId current_scene = p.pathway.OnAScene() ? p.pathway.SceneNearest() : Model::NumScenes;
			do_trigs = false;
			if (current_scene != last_scene_on) {
				last_scene_on = current_scene;
				if (current_scene < Model::NumScenes)
					do_trigs = true;
			}

			for (auto [chan, out] : countzip(buf)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					auto is_primed = ChannelValue::from_volts(0.f);
					if (current_scene < Model::NumScenes)
						is_primed = p.bank.GetChannel(current_scene, chan);
					if (do_trigs && is_primed >= ChannelValue::GateSetThreshold) {
						trigger[chan].Trig(time_now);
					}

					out = trigger[chan].Read(time_now) ? ChannelValue::GateHigh : is_primed;
				} else {
					const auto phs = MathTools::slope_adj(phase, 1.f - p.GetMorph(chan), 0.f, 1.f);
					const auto a = p.bank.GetChannel(left, chan);
					const auto b = p.bank.GetChannel(right, chan);
					out = MathTools::interpolate(a, b, phs);
				}
			}

			do_trigs = true;
		}

		for (auto [i, out] : countzip(buf)) {
			if (p.bank.GetChannelMode(i).IsQuantized())
				out = p.shared.quantizer[i].Process(out);
		}

		return buf;
	}

	Model::OutputBuffer Seq(SeqMode::Interface &p) {
		static auto prev_ = false;
		Model::OutputBuffer buf;

		auto morphamount = std::clamp(p.shared.internalclock.GetPhase(), 0.f, 1.f);
		if (p.seq.player.IsPaused())
			morphamount = 0;
		if (p.shared.internalclock.Output()) {
			// new step
			p.seq.player.Step();
		}

		auto cur = p.shared.internalclock.Peek();

		bool do_trigs = false;
		if (prev_ != cur) {
			prev_ = cur;
			do_trigs = true;
		}
		const auto time_now = p.shared.internalclock.TimeNow();

		for (auto [chan, o] : countzip(buf)) {
			const auto stepvalue = p.seq.GetPlayheadValue(chan);
			auto modifier = p.seq.GetPlayheadModifier(chan);

			if (p.seq.Channel(chan).mode.IsGate()) {
				if (do_trigs && stepvalue >= ChannelValue::GateSetThreshold)
					trigger[chan].Trig(time_now);
				o = trigger[chan].Read(time_now) ? ChannelValue::GateHigh : stepvalue;
			} else {
				const auto nextstepvalue = p.seq.GetNextStepValue(chan);
				const auto distance = nextstepvalue - stepvalue;
				const auto val = stepvalue + (distance * morphamount * modifier.AsMorph());

				auto t = p.shared.quantizer[chan].Process(val);
				o = Transposer::Process(t, p.seq.GetTranspose(chan));
			}
		}
		return buf;
	}
};

} // namespace Catalyst2
