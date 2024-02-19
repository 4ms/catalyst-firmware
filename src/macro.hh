#pragma once

#include "bank.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "pathway.hh"
#include "random.hh"
#include "recorder.hh"
#include "shared.hh"
#include "slider_slew.hh"
#include "util/countzip.hh"
#include "validate.hh"

namespace Catalyst2::Macro
{

struct Data {
	Bank::Data bank{};
	Recorder::Data recorder{};
	SliderSlew::Data slider_slew{};
	Clock::Divider::type clockdiv{};
	bool override_outputs = true;

	bool validate() const {
		auto ret = true;
		ret &= bank.Validate();
		ret &= clockdiv.Validate();
		ret &= recorder.Validate();
		ret &= slider_slew.Validate();
		ret &= validateBool(override_outputs);
		return ret;
	}
};

class Interface {
	Data &data;

public:
	Bank::Interface bank{data.bank};
	Recorder::Interface recorder{data.recorder};
	SliderSlew::Interface slider_slew{data.slider_slew};
	Shared::Interface &shared;
	bool main_mode = true;

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
	}
	void IncOutputOverride(int32_t inc) {
		data.override_outputs = inc > 0 ? true : false;
	}
	bool GetOutputOverride() const {
		return data.override_outputs;
	}
	void IncClockDiv(int32_t inc) {
		data.clockdiv.Inc(inc);
	}
	Clock::Divider::type GetClockDiv() const {
		return data.clockdiv;
	}
	void LoadScales() {
		for (auto [i, q] : countzip(shared.quantizer)) {
			q.Load(this->bank.GetChannelMode(i).GetScale());
		}
	}
};
} // namespace Catalyst2::Macro
