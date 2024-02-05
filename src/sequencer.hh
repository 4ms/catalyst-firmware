#pragma once

#include "channel.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "random.hh"
#include "range.hh"
#include "sequence_phaser.hh"
#include "sequencer_player.hh"
#include "sequencer_settings.hh"
#include "shared.hh"
#include "song_mode.hh"
#include "util/countzip.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>

namespace Catalyst2::Sequencer
{

struct Step : Channel::Value {
	static constexpr auto morphmin = 0u, morphmax = 12u;
	static constexpr auto probmin = Random::Sequencer::Probability::min, probmax = Random::Sequencer::Probability::max;
	static constexpr auto trig_delay_min = -32, trig_delay_max = 32;
	uint8_t morph_retrig : 4;
	Random::Sequencer::Probability::type prob : Random::Sequencer::Probability::usable_bits;
	int8_t trig_delay;

public:
	float ReadTrigDelay() const {
		return trig_delay / static_cast<float>(trig_delay_max);
	}
	float ReadMorph() const {
		return morph_retrig / static_cast<float>(morphmax);
	}
	uint8_t ReadRetrig() const {
		return morph_retrig >> 2;
	}
	Random::Sequencer::Probability::type ReadProbability() const {
		return prob;
	}
	void IncModifier(int32_t inc, bool is_gate) {
		if (is_gate) {
			inc *= 4;
		}
		morph_retrig = std::clamp<int32_t>(morph_retrig + inc, morphmin, morphmax);
	}
	void IncProbability(int32_t inc) {
		prob = std::clamp<int32_t>(prob + inc, probmin, probmax);
	}
	void IncTrigDelay(int32_t inc) {
		trig_delay = std::clamp<int32_t>(trig_delay + inc, trig_delay_min, trig_delay_max - 1);
	}
	bool Validate() const {
		auto ret = true;
		ret &= Channel::Value::Validate();
		// trig_Delay is always good to go... cant validate
		ret &= morph_retrig <= morphmax && prob <= probmax;
		return ret;
	}
};

struct ChannelData : public std::array<Step, Model::MaxSeqSteps> {
	bool Validate() const {
		auto ret = true;
		for (auto &s : *this) {
			ret &= s.Validate();
		}
		return ret;
	}
};

struct Data {
	std::array<Sequencer::ChannelData, Model::NumChans> channel;
	Sequencer::Settings::Data settings;
	Player::Data player;
	SongMode::Data songmode;
	Clock::Divider::type clockdiv{};
	Clock::Bpm::type bpm{};

	bool validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.Validate();
		}
		ret &= clockdiv.Validate();
		ret &= settings.Validate();
		ret &= player.Validate();
		ret &= songmode.Validate();
		ret &= bpm.Validate();
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

public:
	Data &data;
	Clock::Bpm seqclock{data.bpm};
	Shared::Interface &shared;
	Player::Interface player{data.player, data.settings, data.songmode};

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
	}

	void Update(float phase) {
		player.Update(phase, seqclock.GetPhase(), seqclock.Output());
	}

	void Reset(bool stop) {
		seqclock.Reset();
		shared.clockdivider.Reset();
		player.Reset();
		seqclock.Pause(stop);

		// blocks trig for a short period of time
		// TODO: come up with better name
		time_trigged = shared.internalclock.TimeNow();
	}

	void Trig() {
		if (shared.internalclock.TimeNow() - time_trigged >= Clock::BpmToTicks(1200)) {
			if (seqclock.IsInternal()) {
				seqclock.SetExternal(true);
			}
			shared.clockdivider.Update(data.clockdiv);
			if (shared.clockdivider.Step()) {
				seqclock.Input(shared.internalclock.TimeNow());
			}
		}
	}

	void SelectChannel(uint8_t chan) {
		cur_channel = chan;
	}
	uint8_t GetSelectedChannel() {
		return cur_channel;
	}
	void DeselectChannel() {
		cur_channel = Model::NumChans;
	}
	bool IsChannelSelected() {
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
		auto &c = data.channel[cur_channel][StepOnPageToStep(step)];
		if (data.settings.GetChannelMode(cur_channel).IsGate()) {
			if (fine) {
				c.IncTrigDelay(inc);
			} else {
				c.IncGate(inc);
			}
		} else {
			c.IncCv(inc, fine, data.settings.GetRange(cur_channel));
		}
	}
	void IncStepModifier(uint8_t step, int32_t inc) {
		step = StepOnPageToStep(step);
		data.channel[cur_channel][step].IncModifier(inc, data.settings.GetChannelMode(cur_channel).IsGate());
	}
	void IncStepProbability(uint8_t step, int32_t inc) {
		step = StepOnPageToStep(step);
		data.channel[cur_channel][step].IncProbability(inc);
	}

	Channel::Value::Proxy GetPlayheadValue(uint8_t chan) {
		const auto step = player.GetPlayheadStep(chan);
		const auto &s = data.channel[chan][step];
		const auto r = player.randomvalue.Read(chan, s.ReadProbability());
		return s.Read(data.settings.GetRange(chan), r * data.settings.GetRandomOrGlobal(chan));
	}

	Step GetPlayheadModifier(uint8_t chan) {
		const auto step = player.GetPlayheadStep(chan);
		return data.channel[chan][step];
	}

	float GetPlayheadTrigDelay(uint8_t chan) {
		const auto step = player.GetPlayheadStep(chan);
		return data.channel[chan][step].ReadTrigDelay();
	}

	Channel::Value::Proxy GetPrevStepValue(uint8_t chan) {
		const auto step = player.GetPrevPlayheadStep(chan);
		const auto &s = data.channel[chan][step];
		const auto r = player.randomvalue.ReadPrev(chan, s.ReadProbability());
		return s.Read(data.settings.GetRange(chan), r * data.settings.GetRandomOrGlobal(chan));
	}
	std::array<float, Model::SeqStepsPerPage> GetPageValuesTrigDelay(uint8_t page) {
		std::array<float, Model::SeqStepsPerPage> out;
		for (auto [i, o] : countzip(out)) {
			const auto step = (page * Model::SeqStepsPerPage) + i;
			o = data.channel[cur_channel][step].ReadTrigDelay();
		}
		return out;
	}
	Model::Output::Buffer GetPageValuesGate(uint8_t page) {
		Model::Output::Buffer out;
		const auto range = data.settings.GetRange(cur_channel);
		for (auto [i, o] : countzip(out)) {
			const auto step = (page * Model::SeqStepsPerPage) + i;
			o = data.channel[cur_channel][step].Read(range, 0).AsGate() * Channel::range;
		}
		return out;
	}
	Model::Output::Buffer GetPageValuesCv(uint8_t page) {
		Model::Output::Buffer out;
		const auto range = data.settings.GetRange(cur_channel);
		for (auto [i, o] : countzip(out)) {
			const auto step = (page * Model::SeqStepsPerPage) + i;
			o = data.channel[cur_channel][step].Read(range, 0).AsCV();
		}
		return out;
	}
	std::array<float, Model::NumChans> GetPageValuesModifier(uint8_t page) {
		std::array<float, Model::NumChans> out;
		for (auto [i, o] : countzip(out)) {
			o = data.channel[cur_channel][(page * Model::SeqStepsPerPage) + i].ReadMorph();
		}
		return out;
	}
	std::array<float, Model::NumChans> GetPageValuesProbability(uint8_t page) {
		std::array<float, Model::NumChans> out;
		for (auto [i, o] : countzip(out)) {
			o = data.channel[cur_channel][(page * Model::SeqStepsPerPage) + i].ReadProbability() /
				static_cast<float>(Step::probmax);
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

private:
	uint8_t StepOnPageToStep(uint8_t step_on_page) {
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel);
		return step_on_page + (page * Model::SeqStepsPerPage);
	}
};

} // namespace Catalyst2::Sequencer
