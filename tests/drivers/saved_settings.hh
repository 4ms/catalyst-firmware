#pragma once

template<typename SeqModeData, typename MacroModeData, typename SharedData>
class SavedSettings {

	// WearLevel<mdrivlib::FlashBlock<SeqModeData, SeqSettingsFlashAddr, SettingsSectorSize>> seq_settings_flash;
	// WearLevel<mdrivlib::FlashBlock<MacroModeData, MacroSettingsFlashAddr, SettingsSectorSize>>
	// macro_settings_flash;

public:
	bool read(SeqModeData &data) {
		return true;
	}
	bool write(SeqModeData const &data) {
		return true;
	}
	bool read(MacroModeData &data) {
		return true;
	}
	bool write(MacroModeData const &data) {
		return true;
	}
	bool read(SharedData &data) {
		return true;
	}
	bool write(SharedData const &data) {
		return true;
	}
};
