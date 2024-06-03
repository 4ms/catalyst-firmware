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

	void PreSave() {
		slider_slew.PreSave();
	}

	void PostLoad() {
		slider_slew.PostLoad();
	}

	bool validate() const {
		auto ret = true;
		ret &= bank.Validate();
		ret &= clockdiv.Validate();
		ret &= recorder.Validate();
		ret &= slider_slew.Validate();
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
	void Update(uint16_t pos) {
		const auto p = recorder.Update(pos) / 4095.f;
		if (!(shared.youngest_scene_button && (data.override_outputs.mode == Blind::Mode::SLEW))) {
			pos = slew.slider.Update(p);
		}
		bank.pathway.Update(p);
	}
	bool GetOutputOverride() const {
		return data.override_outputs;
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
