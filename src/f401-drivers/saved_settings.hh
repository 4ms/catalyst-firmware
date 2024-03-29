#pragma once
#include "conf/flash_layout.hh"
#include "conf/model.hh"
#include "drivers/flash_block.hh"
#include "drivers/flash_sectors.hh"
#include "legacy/v1_0/legacy_shared_data.hh"
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

	static constexpr uint32_t Version1_1Tag = 0xABCDDCBA;

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
		const auto magic_number_v1_1 = *reinterpret_cast<uint32_t *>(SharedSettingsFlashAddr);
		if (magic_number_v1_1 == Version1_1Tag) {
			return shared_settings_flash.read(data);
		}

		auto legacy_settings_slot0 = *reinterpret_cast<Legacy::V1_0::Shared::Data *>(Legacy::V1_0::FirstSlot);
		auto legacy_settings_slot1 = *reinterpret_cast<Legacy::V1_0::Shared::Data *>(Legacy::V1_0::SecondSlot);
		const Legacy::V1_0::Shared::Data *valid_slot = nullptr;

		if (legacy_settings_slot1.Validate()) {
			valid_slot = &legacy_settings_slot1;
		} else if (legacy_settings_slot0.Validate()) {
			valid_slot = &legacy_settings_slot0;
		}

		if (valid_slot) {
			if (valid_slot->saved_mode == Legacy::V1_0::Shared::Data::Mode::Macro)
				data.saved_mode = Catalyst2::Model::Mode::Macro;
			else
				data.saved_mode = Catalyst2::Model::Mode::Sequencer;

			for (auto [oldcal, newcal] : zip(valid_slot->dac_calibration.channel, data.dac_calibration.channel)) {
				newcal.offset = oldcal.offset;
				newcal.slope = oldcal.slope;
			}

			// wipe legacy sectors, which is v1.1 Settings and Macro sectors
			Legacy::V1_0::eraseFlashSectors();

			// Try to write the extracted data, regardless if the above erasing failed
			// Reset the settings WearLeveling sector since we just wiped it
			auto reset_settings = decltype(shared_settings_flash){};
			data.SettingsVersionTag = Version1_1Tag;
			reset_settings.write(data);

			return true;
		}

		return false;
	}

	bool write(SharedData &data) {
		data.SettingsVersionTag = Version1_1Tag;
		return shared_settings_flash.write(data);
	}
};

} // namespace Catalyst2
