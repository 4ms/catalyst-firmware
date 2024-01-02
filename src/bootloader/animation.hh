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
	}

private:
	Controls &controls;
};
} // namespace Catalyst2::Bootloader

