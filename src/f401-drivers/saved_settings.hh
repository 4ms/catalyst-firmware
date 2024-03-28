#pragma once
#include "conf/flash_layout.hh"
#include "drivers/flash_block.hh"
#include "drivers/flash_sectors.hh"
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
		return ok;
	}

	bool write(SharedData const &data) {
		return shared_settings_flash.write(data);
	}
};

} // namespace Catalyst2
