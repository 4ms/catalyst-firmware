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
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.offset >= min_offset && c.offset <= max_offset;
			ret &= c.slope >= min_slope && c.slope <= max_slope;
		}
		return ret;
	}
};

struct SharedData {
	bool saved_mode = false;
	DacCalibrationData dac_calibration;
	bool validate() const {
		auto ret = true;
		ret &= validateBool(saved_mode);
		ret &= dac_calibration.Validate();
		return ret;
	}
};

struct SharedPlusMacroData {
	static constexpr uint32_t legacy_macro_size = 0x1E8C;
	uint8_t macro_data_padding[legacy_macro_size];

	SharedData shared;

	bool validate() const {
		return shared.validate();
	}
};

constexpr uint32_t block_size = 0x1EAC;
constexpr uint32_t block_padding = 0x4; // due to mdrivlib::FlashBlock adding extra padding
constexpr uint32_t padded_block_size = block_size + block_padding;
constexpr uint32_t block_start_addr = 0x08008000;

using SharedFlashBlock = WearLevel<mdrivlib::FlashBlock<SharedPlusMacroData, block_start_addr, padded_block_size * 2>>;

} // namespace Catalyst2::LegacyV1_0
