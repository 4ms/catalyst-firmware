#pragma once

#include "channel.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "util/countzip.hh"
#include <array>

namespace Catalyst2::Legacy::V1_0::Calibration::Dac
{

// 100 millivolts up or down
inline constexpr auto max_adjustment_volts = .1f;

inline constexpr auto max_offset = Channel::Output::from_volts(-5 + max_adjustment_volts);
inline constexpr auto min_offset = -max_offset;

inline constexpr auto max_slope = Channel::Output::max / Model::output_octave_range * max_adjustment_volts;
inline constexpr auto min_slope = -max_slope;

struct Data {
	struct Channel {
		int16_t offset = 0;
		int16_t slope = 0;
	};
	std::array<Channel, Model::NumChans> channel{};

	bool Validate() const {
		unsigned all_bits_set = true;

		for (auto &c : channel) {
			if (c.offset < min_offset || c.offset > max_offset)
				return false;
			if (c.slope < min_slope || c.slope > max_slope)
				return false;

			if (c.offset != -1 || c.slope != -1)
				all_bits_set = false;
		}

		// If all bits are set, then the data is likely to be erased flash (all 0xFF bytes)
		// So we should reject this data
		if (all_bits_set)
			return false;

		return true;
	}
};
} // namespace Catalyst2::Legacy::V1_0::Calibration::Dac
