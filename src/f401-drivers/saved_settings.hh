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
		bool ok = shared_settings_flash.read(data);

		if (!ok) {
			// __BKPT(1); // This never triggers!
			LegacyV1_0::SharedFlashBlock legacy_flash;
			LegacyV1_0::SharedPlusMacroData legacy_data;

			if (legacy_flash.read(legacy_data)) {

				if (legacy_data.shared.saved_mode)
					data.saved_mode = Model::Mode::Macro;
				else
					data.saved_mode = Model::Mode::Sequencer;

				auto &new_cal = data.dac_calibration.channel;
				auto const &old_cal = legacy_data.shared.dac_calibration.channel;
				for (auto [oldcal, newcal] : zip(old_cal, new_cal)) {
					newcal.offset = oldcal.offset;
					newcal.slope = oldcal.slope;
				}

				ok = true;
			}
		}
		return ok;
	}

	bool write(SharedData const &data) {
		return shared_settings_flash.write(data);
	}
};

} // namespace Catalyst2
