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

class MacroSeq {
	Params &params;

public:
	MacroSeq(Params &params)
		: params{params} {
	}

	auto Update() {
		return params.shared.data.mode == Model::Mode::Macro ? Macro(params.macro) : Seq(params.sequencer);
	}

private:
	Model::Output::Buffer Macro(Macro::Interface &p) {
		// TODO: get rid of these static variables
		static auto do_trigs = false;
		static uint8_t prev = 0;

		Model::Output::Buffer buf;

		const auto time_now = p.shared.internalclock.TimeNow();

		if (p.override_output.has_value()) {
			if (p.override_output.value() != prev) {
				prev = p.override_output.value();
				do_trigs = true;
			}
			for (auto [chan, out] : countzip(buf)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					const auto gate_armed = p.bank.GetChannel(p.override_output.value(), chan).AsGate();
					if (do_trigs && gate_armed) {
					}
				} else {
					out = p.bank.GetChannel(p.override_output.value(), chan).AsCV();
					out = p.bank.GetRange(chan).Clamp(out);
				}
			}
			do_trigs = false;
		} else {
			const auto left = p.pathway.SceneLeft();
			const auto right = p.pathway.SceneRight();

			do_trigs = false;
			const auto current_scene = p.pathway.CurrentScene();
			if (current_scene != p.pathway.LastSceneOn()) {
				if (current_scene.has_value()) {
					do_trigs = true;
				}
			}

			for (auto [chan, out] : countzip(buf)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					auto is_primed = false;
					auto level = Channel::gateoff;
					if (current_scene.has_value()) {
						const auto is_primed = p.bank.GetChannel(current_scene.value(), chan).AsGate();
						if (is_primed) {
							level = Channel::gatearmed;
						}
					}

					if (do_trigs && is_primed) {
					}

				} else {
					const auto phs = MathTools::crossfade_ratio(p.pathway.GetPhase(), p.bank.GetMorph(chan));
					const auto a = p.shared.quantizer[chan].Process(p.bank.GetChannel(left, chan).AsCV());
					const auto b = p.shared.quantizer[chan].Process(p.bank.GetChannel(right, chan).AsCV());
					out = MathTools::interpolate(a, b, phs);
					out = p.bank.GetRange(chan).Clamp(out);
				}
			}

			do_trigs = true;
		}

		return buf;
	}

	Model::Output::Buffer Seq(Sequencer::Interface &p) {
		Model::Output::Buffer buf;

		for (auto [chan, o] : countzip(buf)) {
			o = p.data.settings.GetChannelMode(chan).IsGate() ? SeqTrig(p, chan) : SeqCv(p, chan);
		}

		return buf;
	}

	Model::Output::type SeqTrig(Sequencer::Interface &p, uint8_t chan) {
		const auto stepval = p.GetPlayheadValue(chan);
		const auto armed = stepval.AsGate();
		const auto time_now = p.shared.internalclock.TimeNow();

		return 0;
	}

	Model::Output::type SeqCv(Sequencer::Interface &p, uint8_t chan) {
		const auto stepmorph = seqmorph(p.player.GetStepPhase(chan), p.GetPlayheadModifier(chan).ReadMorph());
		auto stepval = p.shared.quantizer[chan].Process(p.GetPrevStepValue(chan).AsCV());
		const auto distance = p.shared.quantizer[chan].Process(p.GetPlayheadValue(chan).AsCV()) - stepval;
		stepval += (distance * stepmorph);
		stepval = Transposer::Process(stepval, p.data.settings.GetTransposeOrGlobal(chan));
		return p.data.settings.GetRange(chan).Clamp(stepval);
	}
};

} // namespace Catalyst2
