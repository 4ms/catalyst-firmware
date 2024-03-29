#pragma once
#include "conf/flash_layout.hh"
#include "drivers/flash_block.hh"
#include "drivers/flash_sectors.hh"
#include "legacy_shared_data.hh"
#include "util/wear_level.hh"

namespace Catalyst2
{

template<typename SeqModeData, typename MacroModeData, typename SharedData>
class SavedSettings {

	WearLevel<mdrivlib::FlashBlock<SeqModeData, SeqSettingsFlashAddr, SeqSettingsSectorSize>> seq_settings_flash;
	WearLevel<mdrivlib::FlashBlock<MacroModeData, MacroSettingsFlashAddr, MacroSettingsSectorSize>>
		macro_settings_flash;
	WearLevel<mdrivlib::FlashBlock<SharedData, SharedSettingsFlashAddr, SharedSettingsSectorSize>>
		shared_settings_flash;

public:
	bool read(SeqModeData &data) {
		return seq_settings_flash.read(data);
	}

	bool write(SeqModeData const &data) {
		return seq_settings_flash.write(data);
	}

	bool read(MacroModeData &data) {
		return macro_settings_flash.read(data);
	}

	bool write(MacroModeData const &data) {
		return macro_settings_flash.write(data);
	}

	bool read(SharedData &data) {
		Legacy::V1_0::SharedFlashBlock legacy_flash;
		Legacy::V1_0::MacroSharedData legacy_data;

		if (legacy_flash.read(legacy_data)) {

			if (legacy_data.isSharedOk()) {

				data.saved_mode = legacy_data.shared.saved_mode;

				auto &new_cal = data.dac_calibration.channel;
				auto const &old_cal = legacy_data.shared.dac_calibration.channel;
				for (auto [oldcal, newcal] : zip(old_cal, new_cal)) {
					newcal.offset = oldcal.offset;
					newcal.slope = oldcal.slope;
				}

				// wipe legacy sector, which is v1.1 Settings and Macro sectors
				if (!Legacy::V1_0::eraseFlashSectors()) {
					// failed to erase sectors... what to do about that?
					return false; //?
				}
			}

			if (legacy_data.isMacroOk()) {
				// TODO: copy macro data
			}

			return true;
		}

		return shared_settings_flash.read(data);
	}

	bool write(SharedData const &data) {
		return shared_settings_flash.write(data);
	}
};

} // namespace Catalyst2
