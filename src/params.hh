#pragma once

#include "clock.hh"
#include "pathway.hh"
#include "quantizer.hh"
#include "recorder.hh"
#include "scene.hh"
#include "sequencer.hh"
#include <array>
#include <optional>

namespace Catalyst2
{

class SharedInterface {
	using QuantizerArray = std::array<Quantizer::Interface, Model::NumChans>;
	class DisplayHanger {
		static constexpr uint32_t duration = Clock::MsToTicks(2000);
		uint8_t onto;
		uint32_t start_time;

	public:
		void Cancel() {
			onto = 0xff;
		}
		void Set(uint8_t encoder, uint32_t time_now) {
			start_time = time_now;
			onto = encoder;
		}
		std::optional<uint8_t> Check(uint32_t time_now) {
			if (time_now - start_time >= duration)
				onto = 0xff;

			if (onto == 0xff)
				return std::nullopt;

			return onto;
		}
	};
	float pos;

public:
	Clock::Bpm internalclock;
	QuantizerArray quantizer;
	Clock::Divider clockdivider;
	Clock::Divider::type clockdiv;
	RandomPool randompool;
	DisplayHanger hang;

	void SetPos(float pos) {
		this->pos = pos;
	}

	float GetPos() {
		return pos;
	}
};

namespace MacroMode
{
struct Data {
	std::array<Pathway::Data, Model::NumBanks> pathway;
	std::array<Bank::Data, Model::NumBanks> bank;
};

class Interface {
	Data &data;
	std::array<float, Model::NumChans> morph;
	uint8_t cur_bank = 0;

public:
	SharedInterface &shared;
	Pathway::Interface pathway;
	Bank::Interface bank;
	Recorder recorder;
	std::optional<uint8_t> override_output; // this doesnt need to be shared.

	Interface(Data &data, SharedInterface &shared)
		: data{data}
		, shared{shared}
		, bank{shared.randompool} {
		SelectBank(0);
		for (auto &m : morph)
			m = 1.f;
	}
	void SelectBank(uint8_t bank) {
		if (bank >= Model::NumBanks)
			return;

		this->bank.Load(data.bank[bank]);
		this->pathway.Load(data.pathway[bank]);

		for (auto [i, q] : countzip(shared.quantizer))
			q.Load(this->bank.GetChannelMode(i).GetScale());

		cur_bank = bank;
	}
	uint8_t GetSelectedBank() {
		return cur_bank;
	}
	void SetMorph(uint8_t bank, float m) {
		morph[bank] = std::clamp(m, 0.f, 1.f);
	}
	float GetMorph(uint8_t bank) {
		return morph[bank];
	}
	void IncMorph(uint8_t bank, int32_t inc) {
		const auto i = (1.f / 100.f) * inc;
		SetMorph(bank, GetMorph(bank) + i);
	}
};
} // namespace MacroMode

namespace SeqMode
{
struct Data {
	std::array<Sequencer::ChannelData, Model::NumChans> channel;
	Sequencer::Settings::Data settings;
};

class Interface {
	uint8_t cur_channel = 0;
	uint8_t cur_page = Model::SeqPages;
	struct Clipboard {
		Sequencer::ChannelData cd;
		Sequencer::Settings::Channel cs;
		std::array<Sequencer::Step, Model::SeqStepsPerPage> page;
	} clipboard;

public:
	Data &data;
	SharedInterface &shared;
	Sequencer::PlayerInterface player;

	Interface(Data &data, SharedInterface &shared)
		: data{data}
		, shared{shared}
		, player{data.settings} {
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
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel, shared.GetPos());
		step += (page * Model::SeqStepsPerPage);
		data.channel[cur_channel][step].Inc(
			inc, fine, data.settings.GetChannelMode(cur_channel).IsGate(), data.settings.GetRange(cur_channel));
	}
	void IncStepModifier(uint8_t step, int32_t inc) {
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel, shared.GetPos());
		step += (page * Model::SeqStepsPerPage);
		data.channel[cur_channel][step].modifier.Inc(inc);
	}
	Model::Output::type GetStepValue(uint8_t chan, uint8_t step) {
		auto rand = static_cast<int32_t>(shared.randompool.GetSequenceVal(chan, step) *
										 data.settings.GetRandomAmount(chan) * Channel::range);
		if (data.settings.GetChannelMode(chan).IsGate()) {
			// gates not affected by randomness?
			rand = 0;
		}

		const auto temp = data.channel[chan][step].val + rand;
		return std::clamp<int32_t>(temp, Channel::min, Channel::max);
	}
	Model::Output::type GetPlayheadValue(uint8_t chan, float phase) {
		return GetStepValue(chan, player.GetPlayheadStep(chan, phase));
	}
	Sequencer::StepModifier GetPlayheadModifier(uint8_t chan, float phase) {
		return data.channel[chan][player.GetPlayheadStep(chan, phase)].modifier;
	}
	Model::Output::type GetNextStepValue(uint8_t chan, float phase) {
		return GetStepValue(chan, player.GetNextStep(chan, phase));
	}
	Model::Output::Buffer GetPageValues(uint8_t page) {
		Model::Output::Buffer out;
		for (auto [i, o] : countzip(out)) {
			o = GetStepValue(cur_channel, (page * Model::SeqStepsPerPage) + i);
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
	void UpdateRange(uint8_t chan) {
		for (auto &i : data.channel[chan]) {
			i.Update(data.settings.GetChannelMode(chan).IsGate(), data.settings.GetRange(chan));
		}
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
};
} // namespace SeqMode

struct Params {
	enum class Mode : bool { Sequencer, Macro };
	Mode mode = Mode::Macro;

	struct Data {
		SeqMode::Data seq;
		MacroMode::Data macro;
	} data;
	SharedInterface shared;

	SeqMode::Interface sequencer{data.seq, shared};
	MacroMode::Interface macro{data.macro, shared};
};

} // namespace Catalyst2
