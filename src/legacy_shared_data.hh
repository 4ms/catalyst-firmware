#pragma once
#include "channel.hh"
#include "drivers/flash_block.hh"
#include "util/wear_level.hh"
#include "validate.hh"
#include <cstdint>

namespace Catalyst2::LegacyV1_0
{

struct DacCalibrationData {
	static constexpr auto max_adjustment_volts = .1f;

	static constexpr auto max_offset = Channel::Output::from_volts(-5 + max_adjustment_volts);
	static constexpr auto min_offset = -max_offset;

	static constexpr auto max_slope = Channel::Output::max / Model::output_octave_range * max_adjustment_volts;
	static constexpr auto min_slope = -max_slope;

	struct Channel {
		int16_t offset = 0;
		int16_t slope = 0;
	};
	std::array<Channel, 8> channel{};

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

struct SharedData {
	uint8_t saved_mode = 0;
	DacCalibrationData dac_calibration;
	bool validate() const {
		if (!validateBool(saved_mode))
			return false;
		if (!dac_calibration.Validate())
			return false;

		return true;
	}
};

struct SharedPlusMacroData {
	static constexpr uint32_t legacy_macro_size = 0x1E88;
	uint8_t macro_data_padding[legacy_macro_size];

	SharedData shared;
	uint8_t padding[4];

	bool validate() const {
		return shared.validate();
	}
};

constexpr uint32_t block_size = 0x1EAC;
constexpr uint32_t block_padding = 0x4; // due to mdrivlib::FlashBlock adding extra padding
constexpr uint32_t padded_block_size = block_size + block_padding;
constexpr uint32_t block_start_addr = 0x08008000;
constexpr uint32_t legacy_sector_size = 0x18000; // Not an actual sector! But that's what we told it

using SharedFlashBlock = WearLevel<mdrivlib::FlashBlock<SharedPlusMacroData, block_start_addr, legacy_sector_size>>;

} // namespace Catalyst2::LegacyV1_0
