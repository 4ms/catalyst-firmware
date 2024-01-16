#pragma once

#include "conf/model.hh"
#include "sequence_phaser.hh"
#include "sequencer_settings.hh"
#include <array>

namespace Catalyst2::Sequencer::Player
{
struct Data {
	Random::Steps::Data randomsteps;
	bool Validate() const {
		return randomsteps.Validate();
	}
};
class Interface {
	struct State {
		uint8_t playhead_step;
		uint8_t prev_playhead_step;
		uint8_t first_step;
		float step_phase;
		float sequence_phase;
	};
	std::array<State, Model::NumChans> channel;
	Data &data;
	Settings::Data &settings;
	Phaser::Interface phaser{settings.phaser};

public:
	Random::Steps::Interface randomsteps{data.randomsteps};
	Interface(Data &data, Settings::Data &settings)
		: data{data}
		, settings{settings} {
	}
	void Update(float phase, float internal_clock_phase, bool do_step) {
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto l = settings.GetLengthOrGlobal(i);
			const auto pm = settings.GetPlayModeOrGlobal(i);
			const auto actual_length = ActualLength(l, pm);
			if (do_step) {
				phaser.Step(i, actual_length);
			}
			auto &c = channel[i];
			c.first_step = ToStep(i, 0, l, pm);
			c.sequence_phase = GetPhase(i, phase, internal_clock_phase, actual_length);
			const auto playhead = static_cast<uint32_t>(c.sequence_phase);
			c.playhead_step = ToStep(i, playhead, l, pm);
			c.prev_playhead_step = ToStep(i, (playhead - 1u) % actual_length, l, pm);
			c.step_phase = c.sequence_phase - static_cast<int32_t>(c.sequence_phase);
		}
	}
	void Reset() {
		phaser.Reset();
	}
	float GetStepPhase(uint8_t chan) {
		return channel[chan].step_phase;
	}
	uint8_t GetFirstStep(uint8_t chan) {
		const auto l = settings.GetLengthOrGlobal(chan);
		const auto pm = settings.GetPlayModeOrGlobal(chan);
		return ToStep(chan, 0, l, pm);
	}
	uint8_t GetPlayheadPage(uint8_t chan) {
		return channel[chan].playhead_step / Model::SeqPages;
	}
	uint8_t GetPlayheadStepOnPage(uint8_t chan) {
		return channel[chan].playhead_step % Model::SeqPages;
	}
	uint8_t GetPlayheadStep(uint8_t chan) {
		return channel[chan].playhead_step;
	}
	uint8_t GetPrevPlayheadStep(uint8_t chan) {
		return channel[chan].prev_playhead_step;
	}

private:
	float GetPhase(uint8_t chan, float phase, float internal_clock_phase, float actual_length) {
		const auto phase_offset = (settings.GetPhaseOffsetOrGlobal(chan) + phase) * actual_length;
		auto current_phase = phaser.GetPhase(chan, internal_clock_phase) + phase_offset;
		while (current_phase >= actual_length) {
			current_phase -= actual_length;
		}
		return current_phase;
	}
	int32_t ToStep(uint8_t chan, uint32_t step, Settings::Length::type length, Settings::PlayMode::Mode pm) {
		switch (pm) {
			using enum Settings::PlayMode::Mode;
			case Backward:
				step = length + -1 + -step;
				break;
			case Random:
				step = randomsteps.Read(chan, step % length);
				break;
			case PingPong: {
				auto ping = true;
				const auto cmp = length == 1 ? 1u : length - 1u;

				while (step >= cmp) {
					step -= cmp;
					ping = !ping;
				}

				if (!ping) {
					step = length - 1 - step;
				}
				break;
			}
			default:
				break;
		}

		const auto so = settings.GetStartOffsetOrGlobal(chan);
		return ((step % length) + so) % Model::MaxSeqSteps;
	}
};
} // namespace Catalyst2::Sequencer::Player
