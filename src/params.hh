#pragma once
#include "intclock.hh"
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
	using QuantizerArray = std::array<Quantizer<static_cast<uint32_t>(Model::output_octave_range)>, Model::NumChans>;
	class DisplayHanger {
		static constexpr uint32_t time_ms = 2000;
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
			if (time_now - start_time >= 2000)
				onto = 0xff;

			if (onto == 0xff)
				return std::nullopt;

			return onto;
		}
	};
	float pos;
	ClockDivider::type clockdiv = 0;

public:
	Clock::Bpm internalclock;
	QuantizerArray quantizer;
	ClockDivider clockdivider;
	RandomPool randompool;
	DisplayHanger hang;

	void SetPos(float pos) {
		this->pos = pos;
	}

	float GetPos() {
		return pos;
	}

	void IncClockDiv(int32_t inc) {
		clockdiv = ClockDivider::IncDivIdx(clockdiv, inc);
	}

	ClockDivider::type GetClockDiv() {
		return clockdiv;
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
			q.LoadScale(this->bank.GetChannelMode(i).GetScale());

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
using Data = Sequencer::Data;

class Interface {
	uint8_t cur_sequence = 0;
	uint8_t cur_page = Model::SeqPages;

public:
	SharedInterface &shared;
	Sequencer::Interface seq;

	Interface(Data &data, SharedInterface &shared)
		: shared{shared}
		, seq{data, shared.randompool} {
	}

	void SelectSequence(uint8_t sequence) {
		if (sequence >= Model::NumChans)
			return;
		else
			cur_sequence = sequence;
	}
	uint8_t GetSelectedSequence() {
		return cur_sequence;
	}
	void DeselectSequence() {
		cur_sequence = Model::NumChans;
	}
	bool IsSequenceSelected() {
		return cur_sequence != Model::NumChans;
	}
	void SelectPage(uint8_t page) {
		if (page >= Model::SeqPages)
			return;
		else
			cur_page = page;
	}
	uint8_t GetSelectedPage() {
		return cur_page;
	}
	void DeselectPage() {
		cur_page = Model::SeqPages;
	}
	bool IsPageSelected() {
		return cur_page != Model::SeqPages;
	}
	void IncStep(uint8_t step, int32_t inc, bool fine) {
		const auto page = IsPageSelected() ? GetSelectedPage() : seq.GetPlayheadPage(cur_sequence);
		step += (page * Model::SeqStepsPerPage);
		seq.Channel(cur_sequence).IncStep(step, inc, fine);
	}
	void IncStepMorph(uint8_t step, int32_t inc) {
		const auto page = IsPageSelected() ? GetSelectedPage() : seq.GetPlayheadPage(cur_sequence);
		step += (page * Model::SeqStepsPerPage);
		seq.Channel(cur_sequence).IncMorph(step, inc);
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
