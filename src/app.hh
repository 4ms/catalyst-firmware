#pragma once

#include "channel.hh"
#include "conf/build_options.hh"
#include "conf/model.hh"
#include "macro.hh"
#include "params.hh"
#include "trigger.hh"
#include "ui/dac_calibration.hh"
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
	std::optional<uint8_t> last_scene_on;
	bool override = false;
	uint8_t override_scene = 0;
	std::array<Channel::Cv::type, Model::NumChans> last_out;
	std::array<Channel::Cv::type, Model::NumChans> start_point;

public:
	App(Interface &p)
		: p{p} {
	}

	Model::Output::Buffer Update() {
		Model::Output::Buffer cv_output;

		const auto override_phase = p.slew.button.Update();

		// if ((p.shared.youngest_scene_button && p.mode == Mode::Mode::NORMAL) || p.mode == Mode::Mode::LATCH) {
		if (override) {
			// fading to override scene
			const auto scene_b = override_scene;

			const auto current_scene = p.slew.button.IsRunning() ? std::nullopt : std::make_optional(scene_b);
			const auto do_trigs = current_scene != last_scene_on;
			last_scene_on = current_scene;

			for (auto [chan, out, start] : countzip(cv_output, start_point)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					if (do_trigs) {
						trigger.Trig(chan);
					}

					const auto level = current_scene.has_value() ? p.bank.GetGate(current_scene.value(), chan) : 0.f;

					const auto temp = trigger.Read(chan, level) ? Channel::Output::gate_high :
									  level > 0.f				? Channel::Output::gate_armed :
																  Channel::Output::gate_off;
					const auto r = p.bank.GetRange(chan);
					out = Channel::Output::ScaleGate(temp, r);
				} else {
					const auto &scale = p.bank.GetChannelMode(chan).GetScale();
					last_out[chan] = Quantizer::Process(scale, p.bank.GetCv(scene_b, chan));

					const auto chan_morph = p.bank.GetMorph(chan);

					const auto o_phase = MathTools::crossfade_ratio(override_phase, chan_morph);
					const auto temp = MathTools::interpolate(start, last_out[chan], o_phase);

					const auto r = p.bank.GetRange(chan);
					const auto t = Channel::Output::ScaleCv(temp, r);
					out = Calibration::Dac::Process(p.shared.data.dac_calibration.channel[chan], t);
				}
			}
		} else {
			// fading to interpolated scenes
			const auto current_scene = p.slew.button.IsRunning() ? std::nullopt : p.bank.pathway.CurrentGateScene();
			const auto do_trigs = current_scene != last_scene_on;
			last_scene_on = current_scene;

			const auto left_scene = p.bank.pathway.SceneRelative(-1);
			const auto right_scene = p.bank.pathway.SceneRelative(1);

			for (auto [chan, out, start] : countzip(cv_output, start_point)) {
				if (p.bank.GetChannelMode(chan).IsGate()) {
					if (do_trigs) {
						trigger.Trig(chan);
					}

					const auto level = current_scene.has_value() ? p.bank.GetGate(current_scene.value(), chan) : 0.f;

					const auto temp = trigger.Read(chan, level) ? Channel::Output::gate_high :
									  level > 0.f				? Channel::Output::gate_armed :
																  Channel::Output::gate_off;
					const auto r = p.bank.GetRange(chan);
					out = Channel::Output::ScaleGate(temp, r);
				} else {
					const auto &scale = p.bank.GetChannelMode(chan).GetScale();
					const auto left_cv = Quantizer::Process(scale, p.bank.GetCv(left_scene, chan));
					const auto right_cv = Quantizer::Process(scale, p.bank.GetCv(right_scene, chan));

					const auto chan_morph = p.bank.GetMorph(chan);
					const auto crossfader_phase = MathTools::crossfade_ratio(p.bank.pathway.GetPhase(), chan_morph);
					last_out[chan] = MathTools::interpolate(left_cv, right_cv, crossfader_phase);

					const auto o_phase = MathTools::crossfade_ratio(override_phase, chan_morph);
					auto temp = MathTools::interpolate(start, last_out[chan], o_phase);

					const auto r = p.bank.GetRange(chan);
					const auto t = Channel::Output::ScaleCv(temp, r);
					out = Calibration::Dac::Process(p.shared.data.dac_calibration.channel[chan], t);
				}
			}
		}

		CheckEvent();

		return cv_output;
	}

private:
	void CheckEvent() {
		if (p.mode == Mode::Mode::BLIND) {
			override = false;
		} else if (p.mode == Mode::Mode::NORMAL) {
			override = p.shared.youngest_scene_button.has_value();
			if (p.shared.youngest_scene_button.Event()) {
				override_scene = p.shared.youngest_scene_button.value_or(override_scene);
				p.slew.button.Start();
				start_point = last_out;
			}
		} else {
			override = true;
			if (p.shared.youngest_scene_button.Event() && p.shared.youngest_scene_button.has_value()) {
				p.slew.button.Start();
				start_point = last_out;
				override_scene = p.shared.youngest_scene_button.value();
			}
		}
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
			const auto cm = p.slot.settings.GetChannelMode(chan);
			if (cm.IsMuted()) {
				o = Channel::Output::from_volts(0.f);
				continue;
			}
			if (!cm.IsGate()) {
				o = Cv(chan);
				continue;
			}
			if (p.IsGatesBlocked()) {
				o = Channel::Output::gate_off;
				continue;
			}
			o = Gate(chan);
		}

		return buf;
	}

private:
	Model::Output::type Gate(uint8_t chan) {
		bool out = false;

		const auto step_phase = p.player.GetStepPhase(chan);

		auto steps = p.GetStepCluster_TEST(chan);

		std::array order = {-1, 0, 1};

		// if steps overlap, the order that they are output needs to change
		// later steps (in time) should always override steps already in progress
		for (int i = 0; i <= 1; i++) {
			auto p_step = steps[i].ReadTrigDelay();
			p_step *= p.player.RelativeStepMovementDir(chan, i - 1);
			auto step = steps[i + 1].ReadTrigDelay();
			step *= p.player.RelativeStepMovementDir(chan, i);
			if ((p_step - 1.f) > step) {
				std::swap(order[i + 1], order[i]);
				break;
			}
		}

		const auto channel_period_ms = (60.f * 1000.f) / p.GetChannelDividedBpm(chan);

		for (auto i : order) {
			const auto s = steps[i + 1];

			const auto tdelay = s.ReadTrigDelay() * p.player.RelativeStepMovementDir(chan, i);
			const auto s_phase = step_phase - tdelay - i;

			// only output the previous and next step during this step if they are within this steps phase window
			if (s_phase < 0.f || s_phase >= 1.f) {
				continue;
			}

			auto retrig_phase = s.ReadRetrig() + 1.f;

			const auto gate_period_ms = channel_period_ms / retrig_phase;

			auto gate_width_phase = s.ReadGate(p.slot.settings.GetRandomOrGlobal(chan) *
											   p.player.randomvalue.ReadRelative(chan, i, s.ReadProbability()));
			if (gate_width_phase <= 0.f) {
				continue;
			}

			// Clamp gate pulses at 1ms
			const auto gate_width_ms = gate_period_ms * gate_width_phase;
			if (gate_width_ms < 1.f)
				gate_width_phase = 1.f / gate_period_ms;

			retrig_phase *= s_phase;
			retrig_phase -= static_cast<uint32_t>(retrig_phase);

			const auto temp = gate_width_phase >= retrig_phase;
			if constexpr (BuildOptions::seq_gate_overrides_prev_step) {
				out = temp;
			} else {
				out |= temp;
			}
		}

		return Channel::Output::ScaleCv(out ? Channel::Output::gate_high : Channel::Output::gate_off,
										p.slot.settings.GetRange(chan));
	}
	Model::Output::type Cv(uint8_t chan) {
		const auto random = p.slot.settings.GetRandomOrGlobal(chan);
		const auto prev_step = p.GetRelativeStep(chan, -1);
		const auto prev_step_random = p.player.randomvalue.ReadRelative(chan, -1, prev_step.ReadProbability());

		const auto current_step = p.GetRelativeStep(chan, 0);
		const auto current_step_random = p.player.randomvalue.ReadRelative(chan, 0, current_step.ReadProbability());

		const auto &scale = p.slot.settings.GetChannelMode(chan).GetScale();
		auto stepval = Quantizer::Process(scale, prev_step.ReadCv(prev_step_random * random));
		const auto distance = Quantizer::Process(scale, current_step.ReadCv(current_step_random * random)) - stepval;
		const auto stepmorph = seqmorph(p.player.GetStepPhase(chan), current_step.ReadMorph());
		stepval += distance * stepmorph;
		stepval = Transposer::Process(stepval, p.slot.settings.GetTransposeOrGlobal(chan));
		const auto r = p.slot.settings.GetRange(chan);
		const auto temp = Channel::Output::ScaleCv(stepval, r);
		return Calibration::Dac::Process(p.shared.data.dac_calibration.channel[chan], temp);
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
