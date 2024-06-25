#pragma once

#include "bank.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "macro_mode.hh"
#include "pathway.hh"
#include "random.hh"
#include "recorder.hh"
#include "shared.hh"
#include "slew.hh"
#include "util/countzip.hh"
#include "validate.hh"

namespace Catalyst2::Macro
{

struct Data {
	Bank::Data bank{};
	Recorder::Data recorder{};
	Slew::Data slew{};
	Clock::Divider::type clockdiv{};
	Mode::Data mode{};

	void PreSave() {
		slew.PreSave();
	}

	void PostLoad() {
		slew.PostLoad();
	}

	bool validate() const {
		auto ret = true;
		ret &= bank.Validate();
		ret &= clockdiv.Validate();
		ret &= recorder.Validate();
		ret &= slew.Validate();
		ret &= mode.Validate();
		return ret;
	}
};

class Interface {
	Data &data;

public:
	Bank::Interface bank{data.bank};
	Recorder::Interface recorder{data.recorder};
	Slew::Interface slew{data.slew};
	Mode::Interface mode{data.mode};
	Shared::Interface &shared;

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
	}
	void Update(uint16_t pos) {
		auto p = recorder.Update(pos) / 4095.f;
		p = slew.slider.Update(p);
		bank.pathway.Update(p);
	}
	void Trig() {
		if (recorder.IsCued()) {
			shared.clockdivider.Reset();
			recorder.Record();
		} else {
			if (shared.clockdivider.Update(data.clockdiv)) {
				recorder.Reset();
			}
		}
	}
	void IncClockDiv(int32_t inc) {
		data.clockdiv.Inc(inc);
	}
	Clock::Divider::type GetClockDiv() const {
		return data.clockdiv;
	}
	void Clear() {
		bank.Clear();
		recorder.Clear();
		slew.Clear();
		mode.Clear();
	}
	void IncChannelMode(uint8_t channel, int32_t inc) {
		auto &cm = data.bank.bank[data.bank.cur_bank].channelmode[channel];
		do {
			cm.Inc(inc);
		} while (!bank.GetChannelMode(channel).IsGate() && GetScale(channel).size() == 0 && cm.GetScaleIdx() != 0);
	}
	const Quantizer::Scale &GetScale(uint8_t channel) {
		const auto idx = bank.GetChannelMode(channel).GetScaleIdx();
		if (idx >= Quantizer::scale.size()) {
			return shared.data.custom_scale[idx - Quantizer::scale.size()];
		} else {
			return Quantizer::scale[idx];
		}
	}
};
} // namespace Catalyst2::Macro
