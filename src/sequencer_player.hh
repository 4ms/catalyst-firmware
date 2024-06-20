#pragma once

#include "conf/model.hh"
#include "queue.hh"
#include "random.hh"
#include "sequence_phaser.hh"
#include "sequencer_settings.hh"
#include "song_mode.hh"
#include "util/math.hh"
#include <array>
#include <cstdlib>

namespace Catalyst2::Sequencer::Player
{
struct Data {
	Random::Sequencer::Steps::Data randomsteps;
	bool Validate() const {
		return randomsteps.Validate();
	}
};
class Interface {
	struct State {
		float step_phase;
		float sequence_phase;
		std::array<uint8_t, 3> relative_playhead_step;
		std::array<bool, 3> relative_step_backwards;
	};
	std::array<State, Model::NumChans> channel;
	Data &data;
	Settings::Data &settings;
	Phaser::Interface phaser{settings.phaser};

public:
	Queue::Interface queue{settings};
	SongMode::Interface songmode;
	Random::Sequencer::Probability::Interface randomvalue;
	Random::Sequencer::Steps::Interface randomsteps{data.randomsteps};
	Interface(Data &data, Settings::Data &settings, SongMode::Data &songmode)
		: data{data}
		, settings{settings}
		, songmode{songmode} {
	}
	bool pause = false;
	float phase = 0.f;

	void Update(float phase, float internal_clock_phase, bool do_step) {
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			const auto l = settings.GetLengthOrGlobal(chan);
			const auto pm = settings.GetPlayModeOrGlobal(chan);
			const auto actual_length = ActualLength(l, pm);
			if (do_step) {
				if (phaser.Step(chan, actual_length)) {
					if (!songmode.IsActive()) {
						if (songmode.IsQueued(chan)) {
							songmode.Step(chan);
						} else {
							queue.Step(chan);
						}
					} else {
						if (queue.IsQueued(chan)) {
							queue.Step(chan);
						} else {
							songmode.Step(chan);
						}
					}
				}
			}
			auto &c = channel[chan];
			const auto sp = GetPhase(chan, phase, internal_clock_phase, actual_length);

			const auto new_ = static_cast<uint32_t>(sp);
			const auto prev_ = static_cast<uint32_t>(c.sequence_phase);

			// TODO: struggles with phase input.. on reset
			if (new_ != prev_) {
				// sequence has changed
				if (new_ > prev_) {
					if (prev_ == 0 && new_ == actual_length - 1) {
						randomvalue.StepBackward(chan);
					} else {
						randomvalue.Step(chan);
					}
				} else {
					if (new_ == 0 && prev_ == actual_length - 1) {
						randomvalue.Step(chan);
					} else {
						randomvalue.StepBackward(chan);
					}
				}
			}

			c.sequence_phase = sp;
			const auto playhead = static_cast<int32_t>(c.sequence_phase);

			for (auto i = -1; i <= 1; i++) {
				const auto p = (playhead + i) < 0 ? actual_length - 1 : (playhead + i) % actual_length;
				c.relative_step_backwards[i + 1] = pm == Settings::PlayMode::Mode::Backward ||
												   (pm == Settings::PlayMode::Mode::PingPong && p >= (l - 1u));
				c.relative_playhead_step[i + 1] = ToStep(chan, p, l, pm);
			}

			c.step_phase = c.sequence_phase - static_cast<int32_t>(c.sequence_phase);
		}
	}
	void Reset() {
		phaser.Reset();
	}
	int8_t RelativeStepMovementDir(uint8_t chan, int8_t pos) const {
		return channel[chan].relative_step_backwards[pos + 1] ? -1 : 1;
	}
	float GetStepPhase(uint8_t chan) const {
		return channel[chan].step_phase;
	}
	uint8_t GetFirstStep(uint8_t chan) const {
		const auto l = settings.GetLengthOrGlobal(chan);
		const auto pm = settings.GetPlayModeOrGlobal(chan);
		return ToStep(chan, 0, l, pm);
	}
	uint8_t GetPlayheadPage(uint8_t chan) const {
		return channel[chan].relative_playhead_step[1] / Model::Sequencer::NumPages;
	}
	uint8_t GetPlayheadStepOnPage(uint8_t chan) const {
		return channel[chan].relative_playhead_step[1] % Model::Sequencer::NumPages;
	}
	uint8_t GetRelativeStep(uint8_t chan, int8_t position) const {
		return channel[chan].relative_playhead_step[position + 1];
	}

private:
	float GetPhase(uint8_t chan, float phase, float internal_clock_phase, float actual_length) const {
		auto phase_offset = settings.GetPhaseOffsetOrGlobal(chan) + phase;
		phase_offset -= static_cast<int>(phase_offset);
		phase_offset *= actual_length;
		auto current_phase = phaser.GetPhase(chan, internal_clock_phase) + phase_offset;
		while (current_phase >= actual_length) {
			current_phase -= actual_length;
		}
		return current_phase;
	}
	int32_t ToStep(uint8_t chan, uint32_t step, Settings::Length::type length, Settings::PlayMode::Mode pm) const {
		switch (pm) {
			using enum Settings::PlayMode::Mode;
			case Backward:
				step = length + -1 + -step;
				break;
			case Random0:
				step = randomvalue.ReadRelative(chan, step);
				break;
			case Random1:
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

		int so;
		if (const auto temp = settings.GetStartOffset(chan)) {
			so = temp.value();
		} else {
			if (!songmode.IsActive()) {
				if (songmode.IsQueued(chan)) {
					so = songmode.Read(chan);
				} else {
					so = queue.Read(chan);
				}
			} else {
				if (queue.IsQueued(chan)) {
					so = queue.Read(chan);
				} else {
					so = songmode.Read(chan);
				}
			}
		}

		return ((step % length) + so) % Model::Sequencer::Steps::Max;
	}
};
} // namespace Catalyst2::Sequencer::Player
