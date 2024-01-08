#pragma once

#include "channel.hh"
#include "conf/model.hh"
#include "random.hh"
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
	Random::Pool::SeqData randompool{};

	bool validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.Validate();
		}
		ret &= settings.Validate();
		return ret;
	}
};

class Interface {
	uint8_t cur_channel = 0;
	uint8_t cur_page = Model::SeqPages;
	int8_t hide_playhead = Model::SeqStepsPerPage;
	struct Clipboard {
		ChannelData cd;
		Settings::Channel cs;
		std::array<Step, Model::SeqStepsPerPage> page;
	} clipboard;

public:
	Data &data;
	Shared::Interface &shared;
	PlayerInterface player{data.settings};
	Random::Pool::Interface<Random::Pool::SeqData> randompool{data.randompool};

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
	}
	int8_t GetHiddenStep() {
		return hide_playhead;
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
		if (step == player.GetPlayheadStep(cur_channel)) {
			hide_playhead = step;
		} else {
			hide_playhead = -1;
		}
		const auto rand = randompool.Read(cur_channel, step, data.settings.GetRandomOrGlobal(cur_channel));
		data.channel[cur_channel][step].Inc(
			inc, fine, data.settings.GetChannelMode(cur_channel).IsGate(), data.settings.GetRange(cur_channel), rand);
	}
	void IncStepModifier(uint8_t step, int32_t inc) {
		step = StepOnPageToStep(step);
		if (step == player.GetPlayheadStep(cur_channel)) {
			hide_playhead = step;
		} else {
			hide_playhead = -1;
		}
		data.channel[cur_channel][step].modifier.Inc(inc, data.settings.GetChannelMode(cur_channel).IsGate());
	}
	Channel::Value::Proxy GetPlayheadValue(uint8_t chan) {
		const auto step = player.GetPlayheadStep(chan);
		if (step != hide_playhead) {
			hide_playhead = -1;
		}
		return data.channel[chan][step].Read(data.settings.GetRange(chan),
											 randompool.Read(chan, step, data.settings.GetRandomOrGlobal(chan)));
	}
	StepModifier GetPlayheadModifier(uint8_t chan) {
		const auto step = player.GetPlayheadStep(chan);
		if (step != hide_playhead) {
			hide_playhead = -1;
		}
		return data.channel[chan][step].modifier;
	}
	Channel::Value::Proxy GetPrevStepValue(uint8_t chan) {
		const auto step = player.GetPrevStep(chan);
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
				o = data.channel[cur_channel][step].Read(range, rand).AsGate() ? Channel::gatearmed : Channel::gateoff;
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

private:
	uint8_t StepOnPageToStep(uint8_t step_on_page) {
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel);
		return step_on_page + (page * Model::SeqStepsPerPage);
	}
};

} // namespace Catalyst2::Sequencer
