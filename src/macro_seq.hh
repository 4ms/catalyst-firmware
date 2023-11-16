#pragma once
#include "conf/model.hh"
#include "params.hh"
#include "trigger.hh"
#include "util/countzip.hh"
#include "util/math.hh"
#include <algorithm>

namespace MathTools
{
// Given a phase (0..1), return a crossfade amount (0..1)
// ratio is amount of time spent in the crossfade (0..1)
// If ratio is < 0, result is valid but unspecified
constexpr float crossfade_ratio(float phase, float ratio) {
	if (ratio >= 1.0f) {
		return phase < 0.5f ? 0.f : 1.f;
	}

	const auto m = 1.f / (1.f - ratio);
	const auto x = phase - 0.5f;
	const auto y = (m * x) + 0.5f;
	return constrain(y, 0.f, 1.f);
}
} // namespace MathTools

constexpr float seqmorph(float phase, float ratio) {
	ratio = 1.f - ratio;
	const auto lm = .5f + (.5f * ratio);
	if (ratio >= 1.0f) {
		return phase < lm ? 0.f : 1.f;
	}

	const auto m = 1.f / (1.f - ratio);
	const auto x = phase - lm;
	const auto y = (m * x) + 0.5f;
	return MathTools::constrain(y, 0.f, 1.f);
}

namespace Catalyst2
{

class MacroSeq {
	Params &params;
	std::array<Trigger, Model::NumChans> trigger;
	std::array<Retrigger, Model::NumChans> retrigger;

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
					const auto phs = MathTools::crossfade_ratio(phase, 1.f - p.GetMorph(chan));
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
		Model::OutputBuffer buf;

		if (p.shared.internalclock.Output())
			p.seq.player.Step();

		if (p.shared.internalclock.MultOutput()) {
			for (auto &rt : retrigger)
				rt.Update();
		}

		const auto morph_phase = p.seq.player.IsPaused() ? 0.f : p.shared.internalclock.GetPhase();

		for (auto [chan, o] : countzip(buf))
			o = p.seq.Channel(chan).mode.IsGate() ? SeqTrig(p, chan) : SeqCv(p, chan, morph_phase);

		return buf;
	}

	ChannelValue::type SeqTrig(SeqMode::Interface &p, uint8_t chan) {
		const auto stepval = p.seq.GetPlayheadValue(chan);
		const auto armed = stepval >= ChannelValue::GateSetThreshold;
		const auto time_now = p.shared.internalclock.TimeNow();
		if (armed && p.seq.player.IsCurrentStepNew(chan))
			retrigger[chan].Trig(p.seq.GetPlayheadModifier(chan).AsRetrig(), p.seq.Channel(chan).GetClockDiv().Read());

		if (retrigger[chan].Read())
			trigger[chan].Trig(time_now);

		return trigger[chan].Read(time_now) ? ChannelValue::GateHigh :
			   armed						? ChannelValue::GateArmed :
											  ChannelValue::GateOff;
	}

	ChannelValue::type SeqCv(SeqMode::Interface &p, uint8_t chan, float morph_phase) {
		morph_phase = p.seq.player.GetPhase(chan, morph_phase);
		const auto stepmorph = seqmorph(morph_phase, p.seq.GetPlayheadModifier(chan).AsMorph());
		auto stepval = p.seq.GetPlayheadValue(chan);
		const auto distance = p.seq.GetNextStepValue(chan) - stepval;
		stepval += (distance * stepmorph);
		stepval = p.shared.quantizer[chan].Process(stepval);
		return Transposer::Process(stepval, p.seq.GetTranspose(chan));
	}
};

} // namespace Catalyst2
