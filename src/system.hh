#pragma once
#include "conf/flash_layout.hh"
#include "conf/rcc_conf.hh"
#include "debug.hh"
#include "drivers/system.hh"
#include "drivers/system_clocks.hh"
#include "system_target.hh"

namespace Catalyst2
{

struct System {
	System()
	{
		mdrivlib::System::SetVectorTable(AppStartAddr);
		mdrivlib::SystemClocks::init_clocks(osc_conf, clk_conf, rcc_periph_conf);

		Debug::Pin0{};

		SystemTarget::init();

		// __HAL_DBGMCU_FREEZE_TIM6();
		// __HAL_DBGMCU_FREEZE_TIM7();
	}
};

} // namespace Catalyst2
