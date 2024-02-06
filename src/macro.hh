#pragma once

#include "bank.hh"
#include "clock.hh"
#include "pathway.hh"
#include "random.hh"
#include "recorder.hh"
#include "shared.hh"
#include "slider_slew.hh"
#include "util/countzip.hh"

namespace Catalyst2::Macro
{

struct Data {
	std::array<Pathway::Data, Model::NumBanks> pathway{};
	std::array<Bank::Data, Model::NumBanks> bank{};
	Random::Macro::Pool::Data randompool{};
	Recorder::Data recorder{};
	SliderSlew::Data slider_slew{};
	Clock::Divider::type clockdiv{};

	bool validate() {
		auto ret = true;
		for (auto &p : pathway) {
			ret &= p.Validate();
		}
		for (auto &b : bank) {
			ret &= b.Validate();
		}
		ret &= clockdiv.Validate();
		ret &= recorder.Validate();
		ret &= slider_slew.Validate();
		return ret;
	}
};

class Interface {
	Data &data;
	uint8_t cur_bank = 0;

public:
	Pathway::Interface pathway;
	Bank::Interface bank{data.randompool};
	Recorder::Interface recorder{data.recorder};
	SliderSlew::Interface slider_slew{data.slider_slew};
	Shared::Interface &shared;

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
		SelectBank(0);
	}
	void IncClockDiv(int32_t inc) {
		data.clockdiv.Inc(inc);
	}
	Clock::Divider::type GetClockDiv() const {
		return data.clockdiv;
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
} // namespace Catalyst2::Macro
