#pragma once

#include "channel.hh"
#include "conf/build_options.hh"
#include "conf/model.hh"
#include "macro.hh"
#include "params.hh"
#include "sequencer_settings.hh"
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
	const auto lm = .5f - (.5f * ratio);
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
namespace Macro
{
class App {
	Interface &p;
	Trigger trigger;
	uint8_t did_override = Model::NumChans;

public:
	App(Interface &p)
		: p{p} {
	}

	Model::Output::Buffer Update() {
		return (p.main_mode && p.shared.youngest_scene_button.has_value() && p.GetOutputOverride()) ? Override() :
																									  Morph();
	}

private:
	Model::Output::type Trig(bool do_trig, uint8_t chan, uint32_t time_now, float level) {
		if (do_trig && level > 0.f) {
			trigger.Trig(chan, time_now, level);
		}
		return trigger.Read(chan, time_now) ? Channel::Output::gate_high :
			   level > 0.f					? Channel::Output::gate_armed :
											  Channel::Output::gate_off;
	}

	Model::Output::Buffer Override() {
		Model::Output::Buffer out;

		const auto scene = p.shared.youngest_scene_button.value();

		const auto time_now = p.shared.internalclock.TimeNow();
		auto do_trigs = false;

		if (did_override != scene) {
			did_override = scene;
			do_trigs = true;
		}

		for (auto [chan, o] : countzip(out)) {
			if (p.bank.GetChannelMode(chan).IsGate()) {
				const auto level = p.bank.GetGate(scene, chan);
				o = Trig(do_trigs, chan, time_now, level);
			} else {
				o = p.bank.GetCv(scene, chan);
				const auto r = p.bank.GetRange(chan);
				o = Channel::Output::Scale(o, r.Min(), r.Max());
			}
		}
		return out;
	}

	Model::Output::Buffer Morph() {
		did_override = Model::NumChans;
		Model::Output::Buffer out;

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto left = p.bank.pathway.SceneRelative(-1);
		const auto right = p.bank.pathway.SceneRelative(1);

		auto do_trigs = false;

		const auto current_scene = p.bank.pathway.CurrentScene();

		if (current_scene != p.bank.pathway.LastSceneOn() && current_scene.has_value()) {
			do_trigs = true;
		}

		for (auto [chan, o] : countzip(out)) {
			if (p.bank.GetChannelMode(chan).IsGate()) {
				const auto level = current_scene.has_value() ? p.bank.GetGate(current_scene.value(), chan) : 0.f;
				o = Trig(do_trigs, chan, time_now, level);
			} else {
				const auto phs = MathTools::crossfade_ratio(p.bank.pathway.GetPhase(), p.bank.GetMorph(chan));
				const auto a = p.shared.quantizer[chan].Process(p.bank.GetCv(left, chan));
				const auto b = p.shared.quantizer[chan].Process(p.bank.GetCv(right, chan));
				o = MathTools::interpolate(a, b, phs);
				const auto r = p.bank.GetRange(chan);
				o = Channel::Output::Scale(o, r.Min(), r.Max());
			}
		}
		return out;
	}
};
} // namespace Macro

namespace Sequencer
{
class App {
	Interface &p;

public:
	App(Interface &p)
		: p{p} {
	}
	Model::Output::Buffer Update() {
		Model::Output::Buffer buf;

		for (auto [chan, o] : countzip(buf)) {
			o = p.slot.settings.GetChannelMode(chan).IsGate() ? Gate(chan) : Cv(chan);
		}

		return buf;
	}

private:
	Model::Output::type Gate(uint8_t chan) {
		bool out = false;

		const auto step_phase = p.player.GetStepPhase(chan);

		float prev_step_phase = 2.f;
		for (int i = -1; i <= 1; i++) {
			auto s = p.GetRelativeStep(chan, i);
			const auto tdelay = s.ReadTrigDelay() * (p.player.IsRelativeStepMovingBackwards(chan, i) ? -1 : 1);
			auto s_phase = step_phase - tdelay - i;
			if (s_phase >= 0.f && s_phase < 1.f) {
				if (prev_step_phase < s_phase) {
					continue;
				}
				prev_step_phase = s_phase;
				s_phase *= s.ReadRetrig() + 1;
				s_phase -= static_cast<uint32_t>(s_phase);
				const auto gate_val = p.GetRelativeStepGate(chan, i);
				if (gate_val <= 0.f) {
					continue;
				}
				const auto temp = gate_val >= s_phase;
				if constexpr (BuildOptions::seq_gate_overrides_prev_step) {
					out = temp;
				} else {
					out |= temp;
				}
			}
		}

		return out ? Channel::Output::gate_high : Channel::Output::gate_off;
	}
	Model::Output::type Cv(uint8_t chan) {
		const auto stepmorph = seqmorph(p.player.GetStepPhase(chan), p.GetRelativeStep(chan, 0).ReadMorph());
		auto stepval = p.shared.quantizer[chan].Process(p.GetRelativeStepCv(chan, -1));
		const auto distance = p.shared.quantizer[chan].Process(p.GetRelativeStepCv(chan, 0)) - stepval;
		stepval += (distance * stepmorph);
		stepval = Transposer::Process(stepval, p.slot.settings.GetTransposeOrGlobal(chan));
		const auto r = p.slot.settings.GetRange(chan);
		return Channel::Output::Scale(stepval, r.Min(), r.Max());
	}
};
} // namespace Sequencer

class MacroSeq {
	Params &params;
	Sequencer::App sequencer{params.sequencer};
	Macro::App macro{params.macro};

public:
	MacroSeq(Params &params)
		: params{params} {
	}

	auto Update() {
		return params.shared.mode == Model::Mode::Macro ? macro.Update() : sequencer.Update();
	}
};

} // namespace Catalyst2
