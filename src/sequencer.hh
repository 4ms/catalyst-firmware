#pragma once

#include "channelmode.hh"
#include "channelvalue.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "randompool.hh"
#include "transposer.hh"
#include "util/countzip.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace Catalyst2::Sequencer
{
using SequenceId = uint8_t;

enum class PlayMode : int8_t {
	Forward,
	Backward,
	PingPong,
	Random,
	NumPlayModes,
};

class StepModifier {
	int8_t m = 0;

public:
	float AsMorph() {
		return m / 8.f;
	}
	uint8_t AsRetrig() {
		auto out = (m + 1) >> 1;
		return std::clamp(out, 0, 3);
	}
	void Inc(int32_t inc) {
		m = std::clamp<int32_t>(m + inc, 0, 8);
	}
};

enum class OptionalConfig : bool { CanBeNull, Normal };

template<typename T, OptionalConfig Config>
struct OptionalSetting {

	OptionalSetting(T min, T max, T init = T{})
		: val{Config == OptionalConfig::CanBeNull ? std::nullopt : std::make_optional(init)}
		, min{min}
		, max{max} {
	}

	template<typename U>
	void Inc(U inc)
	// TODO: make this work for non-enum and enum types
	//  requires(std::is_convertible_v<T, U>)
	{
		if constexpr (Config == OptionalConfig::CanBeNull) {
			if (!val.has_value()) {
				if (inc > 0)
					val = min;
				return;
			}

			if (inc < 0 && val == min) {
				val = std::nullopt;
				return;
			}
		}
		auto int_val = std::clamp<U>(static_cast<U>(val.value()) + inc, static_cast<U>(min), static_cast<U>(max));
		val = T(int_val);
	}

	std::optional<T> Read() {
		return val;
	}

private:
	std::optional<T> val;
	T min;
	T max;
};

struct Step : Channel {
	StepModifier modifier;
};

class ChannelData {
	std::array<Step, Model::MaxSeqSteps> step;
	Clock::Divider::type clockdiv;
	float randomamount = 0;

public:
	OptionalSetting<float, OptionalConfig::CanBeNull> phase_offset{0.f, .999f};
	OptionalSetting<int8_t, OptionalConfig::CanBeNull> length{Model::MinSeqSteps, Model::MaxSeqSteps};
	OptionalSetting<int8_t, OptionalConfig::CanBeNull> start_offset{0, Model::MaxSeqSteps - 1};
	OptionalSetting<PlayMode, OptionalConfig::CanBeNull> playmode{PlayMode::Forward, PlayMode::Random};

	OptionalSetting<Transposer::type, OptionalConfig::CanBeNull> transposer{Transposer::min, Transposer::max};
	ChannelMode mode;

	ChannelData() {
	}

	Step &GetStep(uint8_t step) {
		return this->step[step];
	}
	Step &GetStep(uint8_t page, uint8_t step) {
		return this->step[(page * Model::SeqStepsPerPage) + step];
	}
	void IncStep(uint8_t step, int32_t inc, bool fine) {
		GetStep(step).Inc(inc, fine, mode.IsGate());
	}
	void IncStep(uint8_t page, uint8_t step, int32_t inc, bool fine) {
		GetStep(page, step).Inc(inc, fine, mode.IsGate());
	}
	void IncModifier(uint8_t step, int32_t inc) {
		GetStep(step).modifier.Inc(inc);
	}
	void IncModifier(uint8_t page, uint8_t step, int32_t inc) {
		GetStep(page, step).modifier.Inc(inc);
	}
	void IncClockDiv(int32_t inc) {
		clockdiv.Inc(inc);
	}
	Clock::Divider::type GetClockDiv() {
		return clockdiv;
	}
	void IncRandomAmount(int32_t inc) {
		auto i = (inc / Model::output_octave_range / 12.f);
		randomamount += i;
		randomamount = std::clamp(randomamount, 0.f, 1.f);
	}
	float GetRandomAmount() {
		return randomamount;
	}
};

struct GlobalData {
	OptionalSetting<float, OptionalConfig::Normal> phase_offset{0.f, .999f, 0.f};
	OptionalSetting<int8_t, OptionalConfig::Normal> length{
		Model::MinSeqSteps, Model::MaxSeqSteps, Model::SeqStepsPerPage};
	OptionalSetting<int8_t, OptionalConfig::Normal> start_offset{0, Model::MaxSeqSteps - 1, 0};
	OptionalSetting<PlayMode, OptionalConfig::Normal> playmode{PlayMode::Forward, PlayMode::Random, PlayMode::Forward};
	OptionalSetting<Transposer::type, OptionalConfig::Normal> transposer{
		Transposer::min, Transposer::max, Transposer::min};
};

struct Data {
	std::array<ChannelData, Model::NumChans> channel;
	GlobalData global;
	float master_phase;
};

class PlayerInterface {
	struct State {
		std::array<uint8_t, Model::MaxSeqSteps> randomstep;
		Clock::Divider clockdivider;
		uint8_t counter = 0;
		uint8_t step = 0;
		uint8_t next_step = 1;
		bool new_step = false;
	};
	std::array<State, Model::NumChans> channel;
	bool pause = false;
	Data &d;

public:
	PlayerInterface(Data &d)
		: d{d} {
	}

	void RandomizeSteps() {
		for (auto i = 0u; i < channel.size(); i++) {
			RandomizeSteps(i);
		}
	}

	void RandomizeSteps(uint8_t chan) {
		// can this be prettier?
		uint32_t *d = reinterpret_cast<uint32_t *>(channel[chan].randomstep.data());
		for (auto i = 0u; i < channel[chan].randomstep.size() / sizeof(uint32_t); i++) {
			d[i] = static_cast<uint32_t>(std::rand());
		}
	}

	void Step() {
		if (pause)
			return;

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
		return ToStep(chan, channel[chan].step);
	}

	uint8_t GetNextStep(uint8_t chan) {
		return ToStep(chan, channel[chan].next_step);
	}
	bool IsCurrentStepNew(uint8_t chan) {
		const auto out = channel[chan].new_step;
		channel[chan].new_step = false;
		return out;
	}
	void TogglePause() {
		pause = !pause;
	}
	void ToggleStop() {
		pause = !pause;
		Reset();
	}
	bool IsPaused() {
		return pause;
	}
	float GetPhase(uint8_t chan, float phase) {
		auto cdiv = d.channel[chan].GetClockDiv();
		phase = phase / cdiv.Read();
		auto cdivphase = channel[chan].clockdivider.GetPhase(cdiv);
		return phase + cdivphase;
	}

private:
	void Step(uint8_t chan) {
		auto &cd = d.channel[chan];
		auto &gd = d.global;
		auto &channel = this->channel[chan];

		channel.clockdivider.Update(cd.GetClockDiv());
		if (!channel.clockdivider.Step())
			return;

		channel.new_step = true;

		channel.step = channel.next_step;
		const auto playmode = cd.playmode.Read().value_or(gd.playmode.Read().value());
		auto length = cd.length.Read().value_or(gd.length.Read().value());
		length += playmode == PlayMode::PingPong ? length - 2 : 0;

		channel.counter += 1;
		if (channel.counter >= length) {
			channel.counter = 0;
		}

		channel.next_step = channel.counter;
	}
	void Reset(uint8_t chan) {
		auto &cd = d.channel[chan];
		auto &gd = d.global;
		auto &c = channel[chan];

		const auto playmode = cd.playmode.Read().value_or(gd.playmode.Read().value());
		auto length = cd.length.Read().value_or(gd.length.Read().value());
		length += playmode == PlayMode::PingPong ? length - 2 : 0;

		c.counter = 0;
		c.clockdivider.Reset();
		c.step = c.counter;
		c.counter += 1;
		c.counter = c.counter >= length ? 0 : c.counter;
		c.next_step = c.counter;

		c.new_step = true;
	}

	uint8_t ToStep(uint8_t chan, uint8_t step) {
		auto &cd = d.channel[chan];
		auto &gd = d.global;
		auto &c = channel[chan];

		const auto l = cd.length.Read().value_or(gd.length.Read().value());
		const auto pm = cd.playmode.Read().value_or(gd.playmode.Read().value());
		const auto actuallength = pm == PlayMode::PingPong ? l + l - 2 : l;

		const auto mpo = d.master_phase * ((actuallength + 1.f) / actuallength);
		const auto po = static_cast<int32_t>((cd.phase_offset.Read().value_or(gd.phase_offset.Read().value()) + mpo) *
											 (actuallength - 1)) %
						actuallength;

		auto o = 0;
		switch (pm) {
			using enum PlayMode;
			case Forward:
				o = step + po;
				break;
			case Backward:
				o = l - 1 - step - po;
				break;
			case Random:
				o = c.randomstep[(step + po) % l];
				break;
			case PingPong: {
				auto s = step + po;
				auto ping = true;

				while (s < 0 || s >= l - 1) {
					s -= l - 1;
					ping = !ping;
				}

				if (ping)
					o = s;
				else
					o = l - 1 - s;
				break;
			}
			default:
				break;
		}
		const auto so = cd.start_offset.Read().value_or(gd.start_offset.Read().value());
		return ((o % l) + so) % Model::MaxSeqSteps;
	}
};

class Interface {
	Data &data;
	RandomPool &randompool;

	struct Clipboard {
		std::array<Step, Model::SeqStepsPerPage> page;
		Sequencer::ChannelData sequence;
	} clipboard;

public:
	PlayerInterface player;

	Interface(Data &data, RandomPool &r)
		: data{data}
		, randompool{r}
		, player{data} {
	}

	Transposer::type GetTranspose(uint8_t chan) {
		return data.channel[chan].transposer.Read().value_or(data.global.transposer.Read().value());
	}

	GlobalData &Global() {
		return data.global;
	}

	ChannelData &Channel(uint8_t c) {
		return data.channel[c];
	}

	void SetMasterPhaseOffset(float o) {
		data.master_phase = o;
	}

	void CopySequence(uint8_t sequence) {
		clipboard.sequence = data.channel[sequence];
	}

	void PasteSequence(uint8_t sequence) {
		data.channel[sequence] = clipboard.sequence;
	}

	void CopyPage(uint8_t sequence, uint8_t page) {
		for (auto [i, p] : countzip(clipboard.page)) {
			p = data.channel[sequence].GetStep(page, i);
		}
	}
	void PastePage(uint8_t sequence, uint8_t page) {
		for (auto [i, p] : countzip(clipboard.page)) {
			data.channel[sequence].GetStep(page, i) = p;
		}
	}

	uint8_t GetPlayheadStep(uint8_t sequence) {
		return player.GetPlayheadStep(sequence);
	}

	uint8_t GetPlayheadStepOnPage(uint8_t sequence) {
		return player.GetPlayheadStep(sequence) % Model::SeqPages;
	}

	uint8_t GetPlayheadPage(uint8_t sequence) {
		return player.GetPlayheadStep(sequence) / Model::SeqPages;
	}

	ChannelValue::type GetStepValue(uint8_t sequence, uint8_t step) {
		auto rand = static_cast<int32_t>(randompool.GetSequenceVal(sequence, step) *
										 data.channel[sequence].GetRandomAmount() * ChannelValue::Range);
		if (data.channel[sequence].mode.IsGate()) {
			// gates not affected by randomness?
			rand = 0;
		}

		auto temp = data.channel[sequence].GetStep(step).val + rand;
		return std::clamp<int32_t>(temp, ChannelValue::Min, ChannelValue::Max);
	}

	ChannelValue::type GetPlayheadValue(uint8_t sequence) {
		return GetStepValue(sequence, GetPlayheadStep(sequence));
	}

	StepModifier GetPlayheadModifier(uint8_t sequence) {
		return data.channel[sequence].GetStep(GetPlayheadStep(sequence)).modifier;
	}

	ChannelValue::type GetNextStepValue(uint8_t sequence) {
		return GetStepValue(sequence, player.GetNextStep(sequence));
	}

	Model::OutputBuffer GetPageValues(uint8_t sequence, uint8_t page) {
		Model::OutputBuffer out;

		for (auto [i, o] : countzip(out))
			o = GetStepValue(sequence, (page * Model::SeqStepsPerPage) + i);

		return out;
	}

	std::array<float, Model::NumChans> GetPageValuesModifier(uint8_t sequence, uint8_t page) {
		std::array<float, Model::NumChans> out;
		for (auto [i, o] : countzip(out))
			o = data.channel[sequence].GetStep(page, i).modifier.AsMorph();
		return out;
	}
};

static constexpr auto seqsize = sizeof(Interface);

} // namespace Catalyst2::Sequencer
