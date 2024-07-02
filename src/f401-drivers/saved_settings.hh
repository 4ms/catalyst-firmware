#pragma once
#include "conf/flash_layout.hh"
#include "conf/model.hh"
#include "drivers/flash_block.hh"
#include "drivers/flash_sectors.hh"
#include "legacy/v1_0/legacy_shared_data.hh"
#include "legacy/v1_1-v1_2/shared.hh"
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

	static constexpr uint32_t CurrentSharedSettingsVersionTag = Legacy::V1_1__V1_2::Shared::tag + 1;

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
		// we know the incoming struct has been constructed using the current version's default values
		const auto magic_number = *reinterpret_cast<uint32_t *>(SharedSettingsFlashAddr);
		if (magic_number == CurrentSharedSettingsVersionTag) {
			return shared_settings_flash.read(data);
		} else if (magic_number == Legacy::V1_1__V1_2::Shared::tag) {
			WearLevel<mdrivlib::FlashBlock<Legacy::V1_1__V1_2::Shared::Data,
										   SharedSettingsFlashAddr,
										   SharedSettingsSectorSize>>
				v1_2Data;
			if (v1_2Data.read(*reinterpret_cast<Legacy::V1_1__V1_2::Shared::Data *>(&data))) {
				// we have the old data in the new struct.
				shared_settings_flash.erase();
				data.SettingsVersionTag = CurrentSharedSettingsVersionTag;
				shared_settings_flash.write(data);
				return shared_settings_flash.read(data);
			}
		} else {
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
				data.SettingsVersionTag = CurrentSharedSettingsVersionTag;
				reset_settings.write(data);

				return true;
			}
		}

		return false;
	}

	bool write(SharedData &data) {
		data.SettingsVersionTag = CurrentSharedSettingsVersionTag;
		return shared_settings_flash.write(data);
	}
};

} // namespace Catalyst2
