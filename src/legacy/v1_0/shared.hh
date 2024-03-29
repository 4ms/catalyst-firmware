#pragma once
#include "legacy/v1_0/dac_calibration.hh"
#include <utility>

namespace Catalyst2::Legacy::V1_0::Shared
{

struct Data {
	enum class Mode : uint8_t { Sequencer, Macro };

	Mode saved_mode;
	Calibration::Dac::Data dac_calibration;

	static bool validateMode(uint8_t in) {
		return in == 0x00 || in == 0x01;
	}

	bool Validate() const {
		if (!validateMode(std::to_underlying(saved_mode)))
			return false;
		if (!dac_calibration.Validate())
			return false;
		return true;
	}
};
} // namespace Catalyst2::Legacy::V1_0::Shared
