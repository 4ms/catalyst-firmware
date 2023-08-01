#pragma once
#include "drivers/stm32xx.h"

namespace Catalyst2
{

struct SystemTarget {
	static void init()
	{
		HAL_Init();
		HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_2);
	}
};

} // namespace Catalyst2
