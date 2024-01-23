#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "queue.hh"
#include "sequencer_settings.hh"
#include "util/countzip.hh"
#include <array>
#include <optional>

namespace Catalyst2::Sequencer
{

class PlayerInterface {
	struct State {
		std::array<uint8_t, Model::MaxSeqSteps> randomstep;
		Clock::Divider clockdivider;
		uint8_t counter = 0;
		uint8_t step = 0;
		uint8_t prev_step = Model::SeqStepsPerPage - 1;
		bool new_step = false;
		bool did_step = false;
		uint8_t playhead = 0;
		uint8_t prev_playhead = 0;
	};
	std::array<State, Model::NumChans> channel;
	bool pause = false;
	Settings::Data &d;
	float phase = 0.f;

	Queue::Data queue_data;

public:
	Queue::Interface queue{queue_data};
	PlayerInterface(Settings::Data &d)
		: d{d} {
	}

	void RandomizeSteps() {
		for (auto i = 0u; i < channel.size(); i++) {
			RandomizeSteps(i);
		}
	}

	void RandomizeSteps(uint8_t chan) {
		// TODO can this be prettier?
		uint32_t *d = reinterpret_cast<uint32_t *>(channel[chan].randomstep.data());
		for (auto i = 0u; i < channel[chan].randomstep.size() / sizeof(uint32_t); i++) {
			d[i] = static_cast<uint32_t>(std::rand());
		}
	}

	void Update(float phase) {
		this->phase = phase;
		for (auto [chan, s] : countzip(channel)) {
			const auto last = s.playhead;
			s.playhead = ToStep(chan, s.step);
			s.prev_playhead = ToStep(chan, s.prev_step);
			if (last != s.playhead || s.did_step) {
				s.new_step = true;
				s.did_step = false;
			}
		}
	}

	void Step() {
		if (pause) {
			return;
		}
		for (auto i = 0u; i < channel.size(); i++) {
			Step(i);
		}
	}

	void Reset() {
		for (auto i = 0u; i < channel.size(); i++) {
			Reset(i);
		}
	}
	uint8_t GetFirstStep(uint8_t chan) {
		return ToStep(chan, 0);
	}

	uint8_t GetPlayheadStep(uint8_t chan) {
		return channel[chan].playhead;
	}

	uint8_t GetPlayheadStepOnPage(uint8_t chan) {
		return GetPlayheadStep(chan) % Model::SeqPages;
	}

	uint8_t GetPlayheadPage(uint8_t chan) {
		return GetPlayheadStep(chan) / Model::SeqPages;
	}
	uint8_t GetPrevStep(uint8_t chan) {
		return channel[chan].prev_playhead;
	}
	bool IsCurrentStepNew(uint8_t chan) {
		const auto out = channel[chan].new_step;
		channel[chan].new_step = false;
		return out;
	}
	void TogglePause() {
		pause = !pause;
		if (!pause) {
			for (auto &i : channel) {
				i.new_step = true;
			}
		}
	}
	void Stop() {
		pause = true;
		Reset();
	}
	bool IsPaused() {
		return pause;
	}
	float GetPhase(uint8_t chan, float phase) {
		const auto cdiv = d.GetClockDiv(chan);
		phase = phase / cdiv.Read();
		const auto cdivphase = channel[chan].clockdivider.GetPhase(cdiv);
		return phase + cdivphase;
	}

private:
	void Step(uint8_t chan) {
		auto &s = this->channel[chan];

		s.clockdivider.Update(d.GetClockDiv(chan));
		if (!s.clockdivider.Step()) {
			return;
		}
		s.did_step = true;

		s.prev_step = s.step;
		s.step = s.counter;
		const auto playmode = d.GetPlayModeOrGlobal(chan);
		const auto length = d.GetLengthOrGlobal(chan);

		s.counter += 1;
		if (s.counter >= ActualLength(length, playmode)) {
			s.counter = 0;
		}
		if (d.GetStartOffset(chan).has_value()) {
			if (s.step == 0) {
				if (queue.channel.IsQueued(chan)) {
					d.SetStartOffset(chan, queue.channel.Step(chan) * Model::SeqStepsPerPage);
				}
			}
			queue.global.Step(chan);
		} else {
			if (s.step == 0) {
				if (queue.global.IsQueued(chan)) {
					queue.global.Step(chan);
					if (queue.global.HasFinished()) {
						d.SetStartOffset(queue.global.Read() * Model::SeqStepsPerPage);
					}
				}
			}
		}
	}
	void Reset(uint8_t chan) {
		auto &c = channel[chan];

		const auto playmode = d.GetPlayModeOrGlobal(chan);
		auto length = d.GetLengthOrGlobal(chan);
		length += playmode == Settings::PlayMode::Mode::PingPong ? length - 2 : 0;

		c.counter = 0;
		c.clockdivider.Reset();
		c.step = c.counter;
		c.counter += 1;
		c.counter = c.counter >= length ? 0 : c.counter;
		c.prev_step = c.step;
	}

	uint8_t ToStep(uint8_t chan, uint8_t step) {
		const auto l = d.GetLengthOrGlobal(chan);
		const auto pm = d.GetPlayModeOrGlobal(chan);
		const auto actuallength = ActualLength(l, pm);
		const auto mpo = static_cast<uint32_t>(phase * actuallength);
		auto po = static_cast<uint32_t>(d.GetPhaseOffsetOrGlobal(chan) * actuallength);
		po = po >= actuallength ? actuallength - 1 : po;

		auto s = step + po + mpo;

		switch (pm) {
			using enum Settings::PlayMode::Mode;
			case Backward:
				s = l + -1 + -s;
				break;
			case Random:
				s = channel[chan].randomstep[s % l];
				break;
			case PingPong: {
				auto ping = true;
				const auto cmp = l == 1 ? 1u : l - 1u;

				while (s >= cmp) {
					s -= cmp;
					ping = !ping;
				}

				if (!ping) {
					s = l - 1 - s;
				}
				break;
			}
			case Forward:
				break;

			default:
				// TODO: assert?
				break;
		}

		int so;
		if (d.GetStartOffset(chan).has_value()) {
			so = d.GetStartOffset(chan).value();
		} else {
			if (queue.global.IsLooping()) {
				so = queue.global.Read(chan) * Model::SeqStepsPerPage;
			} else {
				if (queue.global.HasFinished() || queue.global.IsQueued(chan)) {
					so = d.GetStartOffsetOrGlobal(chan);
				} else {
					so = queue.global.Read() * Model::SeqStepsPerPage;
				}
			}
		}
		return ((s % l) + so) % Model::MaxSeqSteps;
	}

	uint32_t ActualLength(int8_t length, Settings::PlayMode::Mode pm) {
		if (pm == Settings::PlayMode::Mode::PingPong) {
			auto out = length + length - 2;
			if (out < 2) {
				out = 2;
			}
			return out;
		}
		return length;
	}
};
} // namespace Catalyst2::Sequencer
