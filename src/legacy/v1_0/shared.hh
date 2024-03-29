#pragma once

#include "conf/model.hh"
#include "legacy/v1_0/dac_calibration.hh"
#include "validate.hh"
#include <utility>

namespace Catalyst2::Legacy::V1_0::Shared
{

struct Data {
	Model::Mode saved_mode;
	Calibration::Dac::Data dac_calibration;
	bool Validate() const {
		if (!validateBool(std::to_underlying(saved_mode)))
			return false;
		if (!dac_calibration.Validate())
			return false;
		return true;
	}
};
} // namespace Catalyst2::Legacy::V1_0::Shared
