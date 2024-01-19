#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "page_sequencer.hh"
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

public:
	Queue::Channel::Interface channel_queue;
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
				if (s.step == 0) {
					if (channel_queue.IsQueued(chan)) {
						const auto page = channel_queue.Step(chan);
						d.SetStartOffset(chan, page * Model::SeqStepsPerPage);
					}
				}
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
	uint8_t GetFirstStep(std::optional<uint8_t> chan) {
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
		auto &channel = this->channel[chan];

		channel.clockdivider.Update(d.GetClockDiv(chan));
		if (!channel.clockdivider.Step()) {
			return;
		}
		channel.did_step = true;

		channel.prev_step = channel.step;
		channel.step = channel.counter;
		const auto playmode = d.GetPlayModeOrGlobal(chan);
		const auto length = d.GetLengthOrGlobal(chan);

		channel.counter += 1;
		if (channel.counter >= ActualLength(length, playmode)) {
			channel.counter = 0;
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

	uint8_t ToStep(std::optional<uint8_t> chan, uint8_t step) {
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
				s = channel[chan.value_or(0)].randomstep[s % l];
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

		const auto so = d.GetStartOffsetOrGlobal(chan);
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
