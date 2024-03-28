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

namespace Catalyst2::Legacy::V1_0::Macro
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
		return ret;
	}
};
} // namespace Catalyst2::Legacy::V1_0::Macro
