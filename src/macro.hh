#pragma once

#include "bank.hh"
#include "blind.hh"
#include "clock.hh"
#include "conf/model.hh"
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
	Blind::Data override_outputs;

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
		ret &= override_outputs.Validate();
		return ret;
	}
};

class Interface {
	Data &data;

public:
	Bank::Interface bank{data.bank};
	Recorder::Interface recorder{data.recorder};
	Slew::Interface slew{data.slew};
	Blind::Interface blind{data.override_outputs};
	Shared::Interface &shared;
	bool main_mode = true;

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
};
} // namespace Catalyst2::Macro
