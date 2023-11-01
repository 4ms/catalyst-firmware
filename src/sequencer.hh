#pragma once

#include "channelmode.hh"
#include "channelvalue.hh"
#include "clockdivider.hh"
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

class ChannelData {
	std::array<Channel, Model::MaxSeqSteps> step;
	std::array<uint8_t, Model::MaxSeqSteps> morph;
	std::optional<float> phase_offset = std::nullopt;
	std::optional<int8_t> length = std::nullopt;
	std::optional<int8_t> start_offset = std::nullopt;
	std::optional<PlayMode> playmode = std::nullopt;
	ClockDivider::type clockdiv = 0;
	float randomamount = 0;
	// 1.f / 15.f;

public:
	ChannelData() {
		for (auto &m : morph)
			m = 0;
	}
	ChannelMode mode;
	void IncStep(uint8_t step, int32_t inc, bool fine) {
		this->step[step].Inc(inc, fine, mode.IsGate());
	}
	void IncMorph(uint8_t step, int32_t inc) {
		auto temp = morph[step] + inc;
		morph[step] = std::clamp<int32_t>(temp, 0, 100);
	}
	void SetStep(uint8_t step, Channel t) {
		this->step[step] = t;
	}
	void SetMorph(uint8_t step, float m) {
		morph[step] = m * 100.f;
	}
	Channel GetStep(uint8_t step) {
		return this->step[step];
	}
	float GetMorph(uint8_t step) {
		return morph[step] / 100.f;
	}
	void IncLength(int32_t inc) {
		if (!length.has_value()) {
			if (inc > 0)
				length = Model::MinSeqSteps;
			return;
		}

		if (inc < 0 && length.value() == Model::MinSeqSteps) {
			length = std::nullopt;
			return;
		}

		auto t = length.value();
		t += inc;
		length = std::clamp<int8_t>(t, Model::MinSeqSteps, Model::MaxSeqSteps);
	}
	std::optional<int8_t> GetLength() {
		return length;
	}

	void IncStartOffset(int32_t inc) {
		if (!start_offset.has_value()) {
			if (inc > 0)
				start_offset = 0;
			return;
		}

		if (inc < 0 && start_offset.value() == 0) {
			start_offset = std::nullopt;
			return;
		}

		auto t = start_offset.value();
		t += inc;
		start_offset = std::clamp<int8_t>(t, 0, Model::MaxSeqSteps - 1);
	}
	std::optional<int8_t> GetStartOffset() {
		return start_offset;
	}

	void IncPhaseOffset(float inc) {
		if (!phase_offset.has_value()) {
			if (inc > 0)
				phase_offset = 0.f;
			return;
		}

		if (inc < 0 && phase_offset.value() == 0.f) {
			phase_offset = std::nullopt;
			return;
		}

		auto temp = phase_offset.value();
		phase_offset = std::clamp(temp + inc, 0.f, 1.f);
	}
	std::optional<float> GetPhaseOffset() {
		return phase_offset;
	}

	void IncPlayMode(int32_t inc) {
		if (!playmode.has_value()) {
			if (inc > 0)
				playmode = static_cast<PlayMode>(0);
			return;
		}

		if (inc < 0 && playmode.value() == static_cast<PlayMode>(0)) {
			playmode = std::nullopt;
			return;
		}

		auto t = static_cast<int8_t>(playmode.value());
		t += inc;
		t = std::clamp<int8_t>(t, 0, static_cast<int32_t>(PlayMode::NumPlayModes) - 1);
		playmode = static_cast<PlayMode>(t);
	}
	std::optional<PlayMode> GetPlayMode() {
		return playmode;
	}

	void IncClockDiv(int32_t inc) {
		clockdiv = ClockDivider::IncDivIdx(clockdiv, inc);
	}
	ClockDivider::type GetClockDiv() {
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
	float phase_offset = 0;
	int8_t length = Model::SeqStepsPerPage;
	int8_t start_offset = 0;
	PlayMode playmode = PlayMode::Forward;

public:
	void IncLength(int32_t inc) {
		auto temp = length;
		temp += inc;
		length = std::clamp<int8_t>(temp, Model::MinSeqSteps, Model::MaxSeqSteps);
	}
	int8_t GetLength() {
		return length;
	}
	void IncStartOffset(int32_t inc) {
		auto temp = start_offset;
		temp += inc;
		start_offset = std::clamp<int8_t>(temp, 0, Model::MaxSeqSteps - 1);
	}
	int8_t GetStartOffset() {
		return start_offset;
	}
	void IncPhaseOffset(float inc) {
		phase_offset = std::clamp(phase_offset + inc, 0.f, 1.f);
	}
	float GetPhaseOffset() {
		return phase_offset;
	}
	void IncPlayMode(int32_t inc) {
		auto temp = static_cast<int8_t>(playmode);
		temp += inc;
		temp = std::clamp<int8_t>(temp, 0, static_cast<int8_t>(Sequencer::PlayMode::NumPlayModes) - 1);
		playmode = static_cast<Sequencer::PlayMode>(temp);
	}
	PlayMode GetPlayMode() {
		return playmode;
	}
};

struct Data {
	std::array<ChannelData, Model::NumChans> channel;
	GlobalData global;
};

class PlayerInterface {
	std::array<uint8_t, Model::MaxSeqSteps> randomstep;
	ClockDivider clockdivider;
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

		const auto playmode = d.channel[channel].GetPlayMode().value_or(d.global.GetPlayMode());
		auto length = d.channel[channel].GetLength().value_or(d.global.GetLength());
		length += playmode == PlayMode::PingPong ? length - 2 : 0;

		counter += 1;
		if (counter >= length) {
			counter = 0;
		}

		const auto start_offset = d.channel[channel].GetStartOffset().value_or(d.global.GetStartOffset());
		const auto phase_offset =
			static_cast<int8_t>(d.channel[channel].GetPhaseOffset().value_or(d.global.GetPhaseOffset()) * (length - 1));

		next_step = CounterToStep(playmode, counter, start_offset, phase_offset, length);
	}

	void Reset(Data &d, const uint8_t channel) {
		const auto length = d.channel[channel].GetLength().value_or(d.global.GetLength());
		const auto start_offset = d.channel[channel].GetStartOffset().value_or(d.global.GetStartOffset());
		const auto phase_offset =
			static_cast<int8_t>(d.channel[channel].GetPhaseOffset().value_or(d.global.GetPhaseOffset()) * (length - 1));
		auto playmode = d.channel[channel].GetPlayMode().value_or(d.global.GetPlayMode());
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
				auto ol = (l / 2) + 1;
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
			std::array<float, Model::SeqStepsPerPage> morph;
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
			m = data.channel[sequence].GetMorph(offset);
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

	float GetPlayheadMorph(uint8_t sequence) {
		return data.channel[sequence].GetMorph(GetPlayheadStep(sequence));
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

	std::array<float, Model::NumChans> GetPageValuesMorph(uint8_t sequence, uint8_t page) {
		std::array<float, Model::NumChans> out;
		for (auto [i, o] : countzip(out))
			o = data.channel[sequence].GetMorph((page * Model::SeqStepsPerPage) + i);
		return out;
	}
};

static constexpr auto seqsize = sizeof(Interface);

} // namespace Catalyst2::Sequencer