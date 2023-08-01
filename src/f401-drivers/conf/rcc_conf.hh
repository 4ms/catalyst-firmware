#pragma once
#include "drivers/stm32xx.h"

namespace Catalyst2
{

const RCC_OscInitTypeDef osc_conf{
	.OscillatorType = RCC_OSCILLATORTYPE_HSE,
	.HSEState = RCC_HSE_ON,
	.PLL =
		{
			.PLLState = RCC_PLL_ON,
			.PLLSource = RCC_PLLSOURCE_HSE,
			.PLLM = 8,
			.PLLN = 168,
			.PLLP = RCC_PLLP_DIV4,
			.PLLQ = 7,
		},
};

const RCC_ClkInitTypeDef clk_conf{
	.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
	.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
	.AHBCLKDivider = RCC_SYSCLK_DIV1,
	.APB1CLKDivider = RCC_HCLK_DIV2,
	.APB2CLKDivider = RCC_HCLK_DIV1,
};

const RCC_PeriphCLKInitTypeDef rcc_periph_conf = {};

} // namespace Catalyst2
