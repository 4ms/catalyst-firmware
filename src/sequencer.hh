#pragma once

#include "channelmode.hh"
#include "channelvalue.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "randompool.hh"
#include "util/countzip.hh"
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
		return m;
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
	void inc(U amt)
	// TODO: make this work for non-enum and enum types
	//  requires(std::is_convertible_v<T, U>)
	{
		if constexpr (Config == OptionalConfig::CanBeNull) {
			if (!val.has_value()) {
				if (amt > 0)
					val = min;
				return;
			}

			if (amt < 0 && val == min) {
				val = std::nullopt;
				return;
			}
		}
		auto int_val = std::clamp<U>(static_cast<U>(val.value()) + amt, static_cast<U>(min), static_cast<U>(max));
		val = T(int_val);
	}

	std::optional<T> read() {
		return val;
	}

private:
	std::optional<T> val;
	T min;
	T max;
};

class ChannelData {
	std::array<Channel, Model::MaxSeqSteps> step;
	std::array<StepModifier, Model::MaxSeqSteps> modifier;
	Clock::Divider::type clockdiv = 0;
	float randomamount = 0;
	// 1.f / 15.f;

public:
	OptionalSetting<float, OptionalConfig::CanBeNull> phase_offset{0.f, 1.f};
	OptionalSetting<int8_t, OptionalConfig::CanBeNull> length{Model::MinSeqSteps, Model::MaxSeqSteps};
	OptionalSetting<int8_t, OptionalConfig::CanBeNull> start_offset{0, Model::MaxSeqSteps - 1};
	OptionalSetting<PlayMode, OptionalConfig::CanBeNull> playmode{PlayMode::Forward, PlayMode::Random};

	ChannelData() {
	}
	ChannelMode mode;
	void IncStep(uint8_t step, int32_t inc, bool fine) {
		this->step[step].Inc(inc, fine, mode.IsGate());
	}
	void IncModifier(uint8_t step, int32_t inc) {
		modifier[step].Inc(inc);
	}
	void SetStep(uint8_t step, Channel t) {
		this->step[step] = t;
	}
	void SetMorph(uint8_t step, StepModifier m) {
		modifier[step] = m;
	}
	Channel GetStep(uint8_t step) {
		return this->step[step];
	}
	StepModifier GetModifier(uint8_t step) {
		return modifier[step];
	}
	void IncClockDiv(int32_t inc) {
		clockdiv = Clock::Divider::IncDivIdx(clockdiv, inc);
	}
	Clock::Divider::type GetClockDiv() {
		return clockdiv;
	}
	void IncRandomAmount(int32_t inc) {
		auto i = (inc / 15.f / 12.f);
		randomamount += i;
		randomamount = std::clamp(randomamount, 0.f, 1.f);
	}
	float GetRandomAmount() {
		return randomamount;
	}
};

class GlobalData {
public:
	OptionalSetting<float, OptionalConfig::Normal> phase_offset{0.f, 1.f, 0.f};
	OptionalSetting<int8_t, OptionalConfig::Normal> length{
		Model::MinSeqSteps, Model::MaxSeqSteps, Model::SeqStepsPerPage};
	OptionalSetting<int8_t, OptionalConfig::Normal> start_offset{0, Model::MaxSeqSteps - 1, 0};
	OptionalSetting<PlayMode, OptionalConfig::Normal> playmode{PlayMode::Forward, PlayMode::Random, PlayMode::Forward};
};

struct Data {
	std::array<ChannelData, Model::NumChans> channel;
	GlobalData global;
	float master_phase;

	// return the global value if there is no channel value
	float GetPhaseOffset(uint8_t chan) {
		return channel[chan].phase_offset.read().value_or(global.phase_offset.read().value());
	}
	int8_t GetLength(uint8_t chan) {
		return channel[chan].length.read().value_or(global.length.read().value());
	}
	int8_t GetStartOffset(uint8_t chan) {
		return channel[chan].start_offset.read().value_or(global.start_offset.read().value());
	}
	PlayMode GetPlaymode(uint8_t chan) {
		return channel[chan].playmode.read().value_or(global.playmode.read().value());
	}
};

class PlayerInterface {
	std::array<uint8_t, Model::MaxSeqSteps> randomstep;
	Clock::Divider clockdivider;
	uint8_t counter = 0;
	uint8_t step = 0;
	uint8_t next_step = 1;

public:
	PlayerInterface() {
		RandomizeSteps();
	}

	void RandomizeSteps() {
		// can this be prettier?
		uint32_t *d = reinterpret_cast<uint32_t *>(randomstep.data());
		for (auto i = 0u; i < randomstep.size() / sizeof(uint32_t); i++) {
			d[i] = static_cast<uint32_t>(std::rand());
		}
	}

	void Step(Data &d, const uint8_t channel) {
		clockdivider.Update(d.channel[channel].GetClockDiv());
		if (!clockdivider.Step())
			return;

		step = next_step;
		const auto playmode = d.GetPlaymode(channel);
		auto length = d.GetLength(channel);
		length += playmode == PlayMode::PingPong ? length - 2 : 0;

		counter += 1;
		if (counter >= length) {
			counter = 0;
		}

		const auto start_offset = d.GetStartOffset(channel);
		const auto phase_offset = static_cast<int8_t>((d.GetPhaseOffset(channel) + d.master_phase) * (length - 1));

		next_step = CounterToStep(playmode, counter, start_offset, phase_offset, length);
	}

	void Reset(Data &d, const uint8_t channel) {
		const auto length = d.GetLength(channel);
		const auto start_offset = d.GetStartOffset(channel);
		const auto phase_offset = static_cast<int8_t>(d.GetPhaseOffset(channel) * (length - 1));
		auto playmode = d.GetPlaymode(channel);
		playmode = playmode == PlayMode::PingPong ? PlayMode::Forward : playmode;

		counter = 0;
		clockdivider.Reset();
		step = CounterToStep(playmode, counter, start_offset, phase_offset, length);
		counter += 1;
		counter = counter >= length ? 0 : counter;
		next_step = CounterToStep(playmode, counter, start_offset, phase_offset, length);
	}

	uint8_t GetPlayheadStep() {
		return step;
	}

	uint8_t GetNextStep() {
		return next_step;
	}

private:
	uint8_t CounterToStep(PlayMode pm, int8_t c, int8_t so, int8_t po, int8_t l) {
		uint8_t o;
		switch (pm) {
			case PlayMode::Forward:
				o = (c + po) % l;
				break;
			case PlayMode::Backward:
				o = (l - 1 - c + po) % l;
				break;
			case PlayMode::Random:
				o = (randomstep[c] + po) % l;
				break;
			case PlayMode::PingPong: {
				const auto ol = (l / 2) + 1;
				if (c < ol)
					o = (c + po) % ol;
				else
					o = (l - c + po) % ol;
				break;
			}
			default:
				o = 0;
				break;
		}
		return (o + so) % Model::MaxSeqSteps;
	}
};

class Interface {
	std::array<PlayerInterface, Model::NumChans> player;
	Data &data;
	RandomPool &randompool;
	bool pause = false;

	struct Clipboard {
		struct Page {
			std::array<Channel, Model::SeqStepsPerPage> step;
			std::array<StepModifier, Model::SeqStepsPerPage> morph;
		} page;
		Sequencer::ChannelData sequence;
	} clipboard;

public:
	Interface(Data &data, RandomPool &r)
		: data{data}
		, randompool{r} {
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

	void Step() {
		if (pause)
			return;

		for (auto [d, p] : countzip(player))
			p.Step(data, d);
	}

	void Reset() {
		for (auto [d, p] : countzip(player))
			p.Reset(data, d);
	}

	void TogglePause() {
		pause = !pause;
		if (pause)
			Reset();
	}

	bool IsPaused() {
		return pause;
	}

	void CopySequence(uint8_t sequence) {
		clipboard.sequence = data.channel[sequence];
	}

	void PasteSequence(uint8_t sequence) {
		data.channel[sequence] = clipboard.sequence;
	}

	void CopyPage(uint8_t sequence, uint8_t page) {
		for (auto [i, p, m] : countzip(clipboard.page.step, clipboard.page.morph)) {
			const auto offset = (page * Model::SeqStepsPerPage) + i;
			p = data.channel[sequence].GetStep(offset);
			m = data.channel[sequence].GetModifier(offset);
		}
	}
	void PastePage(uint8_t sequence, uint8_t page) {
		for (auto [i, p, m] : countzip(clipboard.page.step, clipboard.page.morph)) {
			const auto offset = (page * Model::SeqStepsPerPage) + i;
			data.channel[sequence].SetStep(offset, p);
			data.channel[sequence].SetMorph(offset, m);
		}
	}

	void RandomizeStepPattern(uint8_t sequence) {
		player[sequence].RandomizeSteps();
	}

	void RandomizeStepPattern() {
		for (auto &p : player)
			p.RandomizeSteps();
	}

	uint8_t GetPlayheadStep(uint8_t sequence) {
		return player[sequence].GetPlayheadStep();
	}

	uint8_t GetPlayheadStepOnPage(uint8_t sequence) {
		return player[sequence].GetPlayheadStep() % Model::SeqPages;
	}

	uint8_t GetPlayheadPage(uint8_t sequence) {
		return player[sequence].GetPlayheadStep() / Model::SeqPages;
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
		return data.channel[sequence].GetModifier(GetPlayheadStep(sequence));
	}

	ChannelValue::type GetNextStepValue(uint8_t sequence) {
		return GetStepValue(sequence, player[sequence].GetNextStep());
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
			o = data.channel[sequence].GetModifier((page * Model::SeqStepsPerPage) + i).AsMorph();
		return out;
	}
};

static constexpr auto seqsize = sizeof(Interface);

} // namespace Catalyst2::Sequencer