#pragma once

#include "clock.hh"
#include "sequencer_settings.hh"
#include "util/countzip.hh"
#include <array>
#include <optional>

namespace Catalyst2::Sequencer
{

inline uint32_t LengthAndPlaymodeToActualLength(int8_t length, Settings::PlayMode::Mode pm) {
	if (pm == Settings::PlayMode::Mode::PingPong) {
		auto out = length + length - 2;
		if (out < 2) {
			out = 2;
		}
		return out;
	}
	return length;
}

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
		float phase = 0.f;
		uint8_t actual_length;
	};
	std::array<State, Model::NumChans> channel;
	bool pause = false;
	Settings::Data &d;
	float phase = 0.f;

public:
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
			s.actual_length = LengthAndPlaymodeToActualLength(d.GetLengthOrGlobal(chan), d.GetPlayModeOrGlobal(chan));
			s.phase = CalculateSequencePhase(s, chan);
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
		if (!pause) {
			for (auto i = 0u; i < channel.size(); i++) {
				Step(i);
			}
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
	float GetSequencePhase(uint8_t chan) {
		return channel[chan].phase;
	}

private:
	float CalculateSequencePhase(const State &c, uint8_t chan) const {
		auto p = c.counter / static_cast<float>(c.actual_length);
		p += this->phase;
		p += d.GetPhaseOffsetOrGlobal(chan);
		p += c.clockdivider.GetPhase(d.GetClockDiv(chan)) * (1.f / c.actual_length);
		return p - static_cast<int32_t>(p);
	}
	void Step(uint8_t chan) {
		auto &channel = this->channel[chan];

		channel.clockdivider.Update(d.GetClockDiv(chan));
		if (!channel.clockdivider.Step()) {
			return;
		}

		channel.step = channel.counter;

		channel.counter += 1;
		if (channel.counter >= channel.actual_length) {
			channel.counter = 0;
		}
	}
	void Reset(uint8_t chan) {
		auto &c = channel[chan];

		c.counter = 0;
		c.clockdivider.Reset();
		c.step = c.counter;
		c.counter = 0;
		c.phase = 0.f;
	}
	uint8_t ToStep(std::optional<uint8_t> chan, uint8_t step) {
		const auto l = d.GetLengthOrGlobal(chan);
		const auto pm = d.GetPlayModeOrGlobal(chan);
		const auto actuallength = LengthAndPlaymodeToActualLength(l, pm);
		const auto mpo = static_cast<uint32_t>(channel[chan.value_or(0)].phase * actuallength);

		auto s = step + mpo;

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
		}

		const auto so = d.GetStartOffsetOrGlobal(chan);
		return ((s % l) + so) % Model::MaxSeqSteps;
	}
};
} // namespace Catalyst2::Sequencer