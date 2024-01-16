#pragma once

#include "channel.hh"
#include "conf/model.hh"
#include "random.hh"
#include "sequence_phaser.hh"
#include "sequencer_player.hh"
#include "sequencer_settings.hh"
#include "shared.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Sequencer
{

class StepModifier {
	static constexpr uint8_t min = 0, max = 12;
	uint8_t m = 0;

public:
	float AsMorph() {
		return m / static_cast<float>(max);
	}
	uint8_t AsRetrig() {
		return m >> 2;
	}
	void Inc(int32_t inc, bool is_gate) {
		if (is_gate) {
			inc *= 4;
		}
		m = std::clamp<int32_t>(m + inc, min, max);
	}
	bool Validate() const {
		return m <= max;
	}
};

struct Step : Channel::Value {
	StepModifier modifier;
};

struct ChannelData : public std::array<Step, Model::MaxSeqSteps> {
	bool Validate() const {
		auto ret = true;
		for (auto &s : *this) {
			ret &= s.modifier.Validate();
			ret &= s.Validate();
		}
		return ret;
	}
};

struct Data {
	std::array<Sequencer::ChannelData, Model::NumChans> channel;
	Sequencer::Settings::Data settings;
	Random::Pool::SeqData randompool;
	Random::Steps::Data randomsteps;

	bool validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.Validate();
		}
		ret &= settings.Validate();
		ret &= player.Validate();
		ret &= randompool.Validate();
		return ret;
	}
};

class Interface {
	uint8_t cur_channel = 0;
	uint8_t cur_page = Model::SeqPages;
	struct Clipboard {
		ChannelData cd;
		Settings::Channel cs;
		std::array<Step, Model::SeqStepsPerPage> page;
	} clipboard;
	uint32_t time_trigged;
	float phase;

	static constexpr auto test = (+0u - 1) % 7;

public:
	Data &data;
	Shared::Interface &shared;
	Phaser::Interface phaser{data.settings.phaser};
	Random::Pool::Interface<Random::Pool::SeqData> randompool{data.randompool};
	Random::Steps::Interface randomsteps{data.randomsteps};

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
	}

	void Reset() {
		shared.internalclock.Reset();
		shared.clockdivider.Reset();
		phaser.Reset();
		time_trigged = shared.internalclock.TimeNow();
	}

	void Trig() {
		if (shared.internalclock.TimeNow() - time_trigged >= Clock::BpmToTicks(1200)) {
			if (shared.internalclock.IsInternal()) {
				shared.internalclock.SetExternal(true);
			}
			shared.clockdivider.Update(shared.data.clockdiv);
			if (shared.clockdivider.Step()) {
				shared.internalclock.Input();
			}
		}
	}

	void SelectChannel(uint8_t chan) {
		cur_channel = chan;
	}
	uint8_t GetSelectedChannel() {
		return cur_channel;
	}
	void DeselectSequence() {
		cur_channel = Model::NumChans;
	}
	bool IsSequenceSelected() {
		return cur_channel < Model::NumChans;
	}
	void SelectPage(uint8_t page) {
		cur_page = page;
	}
	uint8_t GetSelectedPage() {
		return cur_page;
	}
	void DeselectPage() {
		cur_page = Model::SeqPages;
	}
	bool IsPageSelected() {
		return cur_page < Model::SeqPages;
	}
	void IncStep(uint8_t step, int32_t inc, bool fine) {
		step = StepOnPageToStep(step);
		const auto rand = randompool.Read(cur_channel, step, data.settings.GetRandomOrGlobal(cur_channel));
		data.channel[cur_channel][step].Inc(
			inc, fine, data.settings.GetChannelMode(cur_channel).IsGate(), data.settings.GetRange(cur_channel), rand);
	}
	void IncStepModifier(uint8_t step, int32_t inc) {
		step = StepOnPageToStep(step);
		data.channel[cur_channel][step].modifier.Inc(inc, data.settings.GetChannelMode(cur_channel).IsGate());
	}
	Channel::Value::Proxy GetPlayheadValue(uint8_t chan) {
		const auto step = phaser.GetPlayheadStep(chan);
		return data.channel[chan][step].Read(data.settings.GetRange(chan),
											 randompool.Read(chan, step, data.settings.GetRandomOrGlobal(chan)));
	}
	StepModifier GetPlayheadModifier(uint8_t chan) {
		const auto step = phaser.GetPlayheadStep(chan);
		return data.channel[chan][step].modifier;
	}
	Channel::Value::Proxy GetPrevStepValue(uint8_t chan) {
		const auto step = phaser.GetPrevStep(chan);
		return data.channel[chan][step].Read(data.settings.GetRange(chan),
											 randompool.Read(chan, step, data.settings.GetRandomOrGlobal(chan)));
	}
	Model::Output::Buffer GetPageValues(uint8_t page) {
		Model::Output::Buffer out;
		const auto range = data.settings.GetRange(cur_channel);
		for (auto [i, o] : countzip(out)) {
			const auto step = (page * Model::SeqStepsPerPage) + i;
			const auto rand = randompool.Read(cur_channel, step, data.settings.GetRandomOrGlobal(cur_channel));
			if (data.settings.GetChannelMode(cur_channel).IsGate()) {
				o = data.channel[cur_channel][step].Read(range, rand).AsGate() * Channel::range;
			} else {
				o = data.channel[cur_channel][step].Read(range, rand).AsCV();
			}
		}
		return out;
	}
	std::array<float, Model::NumChans> GetPageValuesModifier(uint8_t page) {
		std::array<float, Model::NumChans> out;
		for (auto [i, o] : countzip(out)) {
			o = data.channel[cur_channel][(page * Model::SeqStepsPerPage) + i].modifier.AsMorph();
		}
		return out;
	}
	void CopySequence() {
		clipboard.cd = data.channel[cur_channel];
		clipboard.cs = data.settings.Copy(cur_channel);
	}
	void PasteSequence() {
		data.channel[cur_channel] = clipboard.cd;
		data.settings.Paste(cur_channel, clipboard.cs);
	}
	void CopyPage(uint8_t page) {
		for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
			clipboard.page[i] = data.channel[cur_channel][(page * Model::SeqStepsPerPage) + i];
		}
	}
	void PastePage(uint8_t page) {
		for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
			data.channel[cur_channel][(page * Model::SeqStepsPerPage) + i] = clipboard.page[i];
		}
	}
	void Update() {
		const auto ico = shared.internalclock.Output();
		const auto icp = shared.internalclock.GetPhase();
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto l = data.settings.GetLengthOrGlobal(i);
			const auto pm = data.settings.GetPlayModeOrGlobal(i);
			const auto actual_length = ActualLength(l, pm);
			if (ico) {
				phaser.Step(i, actual_length);
			}
			const auto phase = GetSeqPhase(i, icp, actual_length);
			const auto playhead = static_cast<uint32_t>(phase);
			const auto playhead_step = ToStep(i, playhead, l, pm);
			const auto prev_step = ToStep(i, (playhead - 1u) % actual_length, l, pm);
			const auto step_phase = phase - static_cast<int32_t>(phase);
		}
	}

private:
	uint8_t StepOnPageToStep(uint8_t step_on_page) {
		const auto page = IsPageSelected() ? GetSelectedPage() : phaser.GetPlayheadPage(cur_channel);
		return step_on_page + (page * Model::SeqStepsPerPage);
	}
	//////////
	uint32_t ActualLength(Settings::Length::type length, Settings::PlayMode::Mode pm) {
		if (pm != Settings::PlayMode::Mode::PingPong) {
			return length;
		}
		const auto out = length + length - 2;
		return out < 2 ? 2 : out;
	}
	float GetSeqPhase(uint8_t chan, float internal_clock_phase, uint8_t actual_length) {
		const auto phase_offset = (data.settings.GetPhaseOffsetOrGlobal(chan) + phase) * actual_length;
		auto current_phase = phaser.GetPhase(chan, internal_clock_phase) + phase_offset;
		return std::fmodf(current_phase, actual_length);
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

		const auto so = data.settings.GetStartOffsetOrGlobal(chan);
		return ((step % length) + so) % Model::MaxSeqSteps;
	}
};

} // namespace Catalyst2::Sequencer
