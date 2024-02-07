#pragma once

#include "channel.hh"
#include "conf/model.hh"
#include "macro.hh"
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
	uint8_t prev = 0;

public:
	App(Interface &p)
		: p{p} {
	}

	Model::Output::Buffer Update() {
		Model::Output::Buffer buf;

		const auto time_now = p.shared.internalclock.TimeNow();

		auto do_trigs = false;

		if (p.shared.youngest_scene_button.has_value()) {
			if (p.shared.youngest_scene_button.value() != prev) {
				prev = p.shared.youngest_scene_button.value();
				do_trigs = true;
			}
			for (auto [chan, out] : countzip(buf)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					const auto level = p.bank.GetChannel(p.shared.youngest_scene_button.value(), chan).AsGate();
					if (do_trigs && level > 0.f) {
						trigger.Trig(chan, time_now, level);
					}
					out = trigger.Read(chan, time_now) ? Channel::gatehigh :
						  level > 0.f				   ? Channel::gatearmed :
														 Channel::gatehigh;
				} else {
					out = p.bank.GetChannel(p.shared.youngest_scene_button.value(), chan).AsCV();
					out = p.bank.GetRange(chan).Clamp(out);
				}
			}
		} else {
			const auto left = p.pathway.SceneRelative(-1);
			const auto right = p.pathway.SceneRelative(1);

			const auto current_scene = p.pathway.CurrentScene();
			if (current_scene != p.pathway.LastSceneOn()) {
				if (current_scene.has_value()) {
					do_trigs = true;
				}
			}

			for (auto [chan, out] : countzip(buf)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					const auto level =
						current_scene.has_value() ? p.bank.GetChannel(current_scene.value(), chan).AsGate() : 0.f;

					if (do_trigs && level > 0.f) {
						trigger.Trig(chan, time_now, level);
					}

					out = trigger.Read(chan, time_now) ? Channel::gatehigh :
						  level > 0.f				   ? Channel::gatearmed :
														 Channel::gatehigh;

				} else {
					const auto phs = MathTools::crossfade_ratio(p.pathway.GetPhase(), p.bank.GetMorph(chan));
					const auto a = p.shared.quantizer[chan].Process(p.bank.GetChannel(left, chan).AsCV());
					const auto b = p.shared.quantizer[chan].Process(p.bank.GetChannel(right, chan).AsCV());
					out = MathTools::interpolate(a, b, phs);
					out = p.bank.GetRange(chan).Clamp(out);
				}
			}
		}

		return buf;
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
			o = p.data.settings.GetChannelMode(chan).IsGate() ? Gate(chan) : Cv(chan);
		}

		return buf;
	}

private:
	Model::Output::type Gate(uint8_t chan) {
		bool out = false;

		const auto step_phase = p.player.GetStepPhase(chan);

		for (int i = -1; i <= 1; i++) {
			auto s = p.GetRelativeStep(chan, i);
			auto s_phase = step_phase - s.ReadTrigDelay() - i;
			if (s_phase >= 0.f && s_phase < 1.f) {
				s_phase *= s.ReadRetrig() + 1;
				s_phase -= static_cast<uint32_t>(s_phase);
				const auto gate_val = p.GetRelativeStepValue(chan, i).AsGate();
				if (gate_val <= 0.f) {
					continue;
				}
				const auto temp = gate_val >= s_phase;
				if constexpr (Model::seq_gate_overrides_prev_step) {
					out = temp;
				} else {
					out |= temp;
				}
			}
		}

		return out ? Channel::gatehigh : Channel::gateoff;
	}
	Model::Output::type Cv(uint8_t chan) {
		const auto stepmorph = seqmorph(p.player.GetStepPhase(chan), p.GetRelativeStep(chan, 0).ReadMorph());
		auto stepval = p.shared.quantizer[chan].Process(p.GetRelativeStepValue(chan, -1).AsCV());
		const auto distance = p.shared.quantizer[chan].Process(p.GetRelativeStepValue(chan, 0).AsCV()) - stepval;
		stepval += (distance * stepmorph);
		stepval = Transposer::Process(stepval, p.data.settings.GetTransposeOrGlobal(chan));
		return p.data.settings.GetRange(chan).Clamp(stepval);
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
