#pragma once
#include "controls.hh"
#include "drivers/stm32xx.h"
#include "libhwtests/HardwareTestUtil.hh"

namespace Catalyst2::HWTests
{
struct UtilIF {
	static inline Controls *controls;

	static void link_controls(Controls &c)
	{
		controls = &c;
	}

	static bool main_button_pressed()
	{
		return controls->get_scene_button(0).is_pressed();
	}

	static void delay_ms(uint32_t ms)
	{
		HAL_Delay(ms);
	}

	static void set_main_button_led(bool turn_on)
	{
		controls->set_button_led(0, turn_on);
	}
};

using Util = HardwareTestUtil<UtilIF>;
} // namespace Catalyst2::HWTests
