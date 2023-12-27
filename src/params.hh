#pragma once

#include "clock.hh"
#include "macro.hh"
#include "pathway.hh"
#include "quantizer.hh"
#include "recorder.hh"
#include "sequencer.hh"
#include "util/countzip.hh"
#include <array>
#include <optional>

namespace Catalyst2
{

struct Params;

class SharedInterface {
	using QuantizerArray = std::array<Quantizer::Interface, Model::NumChans>;
	class DisplayHanger {
		static constexpr uint32_t duration = Clock::MsToTicks(4000);
		uint8_t onto;
		uint32_t start_time;
		Clock::Bpm &internalclock;

	public:
		DisplayHanger(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Cancel() {
			onto = 0xff;
		}
		void Set(uint8_t encoder) {
			start_time = internalclock.TimeNow();
			onto = encoder;
		}
		std::optional<uint8_t> Check() {
			if (internalclock.TimeNow() - start_time >= duration) {
				onto = 0xff;
			}
			if (onto == 0xff) {
				return std::nullopt;
			}
			return onto;
		}
	};
	class ResetManager {
		static constexpr auto hold_duration = Clock::MsToTicks(3000);
		Clock::Bpm &internalclock;
		uint32_t set_time;
		bool notify = false;

	public:
		ResetManager(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Notify(bool on) {
			notify = on;
			if (on) {
				set_time = internalclock.TimeNow();
			}
		}
		bool Check() {
			if (notify == false || internalclock.TimeNow() - set_time < hold_duration) {
				return false;
			}
			return true;
		}
	};
	class ModeSwitcher {
		static constexpr auto hold_duration = Clock::MsToTicks(3000);
		Clock::Bpm &internalclock;
		uint32_t set_time;

	public:
		ModeSwitcher(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Notify() {
			set_time = internalclock.TimeNow();
		}
		bool Check() {
			return internalclock.TimeNow() - set_time >= hold_duration;
		}
	};

public:
	Clock::Bpm internalclock;
	QuantizerArray quantizer;
	Clock::Divider clockdivider;
	Clock::Divider::type clockdiv;
	DisplayHanger hang{internalclock};
	ResetManager reset{internalclock};
	ModeSwitcher modeswitcher{internalclock};
	float pos;
};

namespace Macro
{

struct Data {
	friend class Catalyst2::Params;

	std::array<Pathway::Data, Model::NumBanks> pathway{};
	std::array<Bank::Data, Model::NumBanks> bank{};
	Random::Pool::MacroData randompool{};

	bool validate() {
		auto ret = true;
		for (auto &p : pathway) {
			ret &= p.Validate();
		}
		for (auto &b : bank) {
			ret &= b.Validate();
		}
		ret &= (saved_mode == Model::Mode::Macro || saved_mode == Model::Mode::Sequencer);
		return ret;
	}

private:
	Model::Mode saved_mode = Model::default_mode;
};

class Interface {
	Data &data;
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
		, bank{data.randompool} {
		SelectBank(0);
	}
	void SelectBank(uint8_t bank) {
		if (bank >= Model::NumBanks) {
			return;
		}
		this->bank.Load(data.bank[bank]);
		this->pathway.Load(data.pathway[bank]);

		for (auto [i, q] : countzip(shared.quantizer)) {
			q.Load(this->bank.GetChannelMode(i).GetScale());
		}
		cur_bank = bank;
	}
	uint8_t GetSelectedBank() {
		return cur_bank;
	}
	void Reset() {
		data.bank[cur_bank] = Bank::Data{};
		data.pathway[cur_bank] = Pathway::Data{};
	}
	void Reset(uint8_t scene) {
		data.bank[cur_bank].scene[scene] = Bank::Data::Scene{};
	}
};
} // namespace Macro

namespace Sequencer
{
class Interface {
	uint8_t cur_channel = 0;
	uint8_t cur_page = Model::SeqPages;
	struct Clipboard {
		ChannelData cd;
		Settings::Channel cs;
		std::array<Step, Model::SeqStepsPerPage> page;
	} clipboard;

public:
	Data &data;
	SharedInterface &shared;
	PlayerInterface player{data.settings};
	Random::Pool::Interface<Random::Pool::SeqData> randompool{data.randompool};

	Interface(Data &data, SharedInterface &shared)
		: data{data}
		, shared{shared} {
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
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel);
		step += (page * Model::SeqStepsPerPage);
		const auto rand = randompool.Read(cur_channel, step, data.settings.GetRandomAmount(cur_channel));
		data.channel[cur_channel][step].Inc(
			inc, fine, data.settings.GetChannelMode(cur_channel).IsGate(), data.settings.GetRange(cur_channel), rand);
	}
	void IncStepModifier(uint8_t step, int32_t inc) {
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel);
		step += (page * Model::SeqStepsPerPage);
		data.channel[cur_channel][step].modifier.Inc(inc, data.settings.GetChannelMode(cur_channel).IsGate());
	}
	Channel::Value::Proxy GetPlayheadValue(uint8_t chan) {
		const auto step = player.GetPlayheadStep(chan);
		return data.channel[chan][step].Read(data.settings.GetRange(chan),
											 randompool.Read(chan, step, data.settings.GetRandomAmount(chan)));
	}
	StepModifier GetPlayheadModifier(uint8_t chan) {
		return data.channel[chan][player.GetPlayheadStep(chan)].modifier;
	}
	Channel::Value::Proxy GetPrevStepValue(uint8_t chan) {
		const auto step = player.GetPrevStep(chan);
		return data.channel[chan][step].Read(data.settings.GetRange(chan),
											 randompool.Read(chan, step, data.settings.GetRandomAmount(chan)));
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
	Model::Output::type GetStepValue(uint8_t chan, uint8_t step) {
		const auto rand = randompool.Read(chan, step, data.settings.GetRandomAmount(chan));
		const auto range = data.settings.GetRange(chan);

		if (data.settings.GetChannelMode(chan).IsGate()) {
			return data.channel[chan][step].Read(range, rand).AsGate();
		} else {
			return data.channel[chan][step].Read(range, rand).AsCV();
		}
	}
};
} // namespace Sequencer

struct Params {
	struct Data {
		Sequencer::Data seq;
		Macro::Data macro;
	};

	Data data;
	Model::Mode &mode = data.macro.saved_mode;
	SharedInterface shared;
	Sequencer::Interface sequencer{data.seq, shared};
	Macro::Interface macro{data.macro, shared};
};

} // namespace Catalyst2
