#pragma once
#include "bootloader-controls.hh"

namespace Catalyst2::Bootloader
{
enum class Animation {
	WAITING,
	WRITING,
	RECEIVING,
	SYNC,
	DONE,
	SUCCESS,
	FAIL_ERR,
	FAIL_SYNC,
	FAIL_CRC,
	RESET,
};

struct LedAnimation {

	LedAnimation(Controls &controls)
		: controls{controls} {
	}

	void animate(Animation animation_type) {
		uint32_t cur_tm = HAL_GetTick();
		uint32_t step_time = 500;

		switch (animation_type) {

			case Animation::RESET:
				for (auto i = 0u; i < 8; i++)
					controls.SetButtonLed(i, false);
				controls.SetPlayLed(false);

				last_tm = cur_tm;
				ctr = 0;
				break;

			case Animation::SUCCESS:
				if (ctr >= Model::NumChans)
					ctr = 0;

				if (ctr == 0) {
					controls.SetButtonLed(Model::NumChans - 1, false);
					controls.SetButtonLed(ctr, true);
				} else {
					controls.SetButtonLed(ctr - 1, false);
					controls.SetButtonLed(ctr, true);
				}
				break;

			case Animation::WAITING:
				if (ctr == 0) {
					controls.SetPlayLed(true);
					controls.SetButtonLed(0, true);
				} else if (ctr == 1) {
					controls.SetPlayLed(false);
					controls.SetButtonLed(0, false);
				} else
					ctr = 0;
				break;

			case Animation::RECEIVING:
				step_time = 200;
				if (ctr == 0) {
					controls.SetButtonLed(0, true);
					controls.SetButtonLed(4, false);
				} else if (ctr == 1) {
					controls.SetButtonLed(0, false);
					controls.SetButtonLed(4, true);
				} else
					ctr = 0;
				break;

			case Animation::SYNC:
				step_time = 100;
				if (ctr == 0) {
					controls.SetButtonLed(1, true);
					controls.SetButtonLed(5, false);
				} else if (ctr == 1) {
					controls.SetButtonLed(1, false);
					controls.SetButtonLed(5, true);
				} else
					ctr = 0;
				break;

			case Animation::WRITING:
				step_time = 100;
				if (ctr == 0) {
					controls.SetButtonLed(2, true);
					controls.SetButtonLed(6, false);
				} else if (ctr == 1) {
					controls.SetButtonLed(2, false);
					controls.SetButtonLed(6, true);
				} else
					ctr = 0;
				break;

			case Animation::FAIL_ERR:
				step_time = 100;
				if (ctr == 0) {
					controls.SetButtonLed(0, true);
					controls.SetButtonLed(1, true);
					controls.SetButtonLed(2, true);
					controls.SetButtonLed(3, true);
				} else if (ctr == 1) {
					controls.SetButtonLed(0, false);
					controls.SetButtonLed(1, false);
					controls.SetButtonLed(2, false);
					controls.SetButtonLed(3, false);
				} else
					ctr = 0;
				break;

			default:
				break;
		}

		if ((cur_tm - last_tm) > step_time) {
			ctr++;
			last_tm = cur_tm;
		}
	}

private:
	Controls &controls;
	uint32_t last_tm = 0;
	uint8_t ctr = 0;
};
} // namespace Catalyst2::Bootloader

