#pragma once

#include "clock.hh"
#include "conf/build_options.hh"
#include "conf/model.hh"
#include "dac_calibration.hh"
#include "quantizer.hh"
#include "validate.hh"
#include <optional>
#include <utility>

namespace Catalyst2::Legacy::V1_0::Shared
{

struct Data {
	Model::Mode saved_mode = BuildOptions::default_mode;
	Calibration::Dac::Data dac_calibration;
	bool Validate() const {
		auto ret = true;
		ret &= validateBool(std::to_underlying(saved_mode));
		ret &= dac_calibration.Validate();
		return ret;
	}
};

struct Dummy {
	bool Validate() const {
		return true;
	}
};
} // namespace Catalyst2::Legacy::V1_0::Shared
