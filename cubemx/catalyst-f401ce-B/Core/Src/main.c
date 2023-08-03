#include "main.h"
#include "stm32f4xx_hal_i2c.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);

//
// LEDs
//

HAL_StatusTypeDef write_leddrv(uint8_t reg, uint8_t data)
{
	HAL_StatusTypeDef err;

	err = HAL_I2C_Mem_Write(&hi2c1, 0b01010000, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 0xFFFF);
	if (err != HAL_OK)
		HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_SET);

	return err;
}

void init_leds()
{
	// Use PA2 to indicate an error
	HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_RESET);

	// enable chip
	write_leddrv(0, (1 << 6));

	// dither + auto inc + no auto power save + log
	write_leddrv(1, 0x2C);

	// no bank mode
	write_leddrv(2, 0);

	// all lights off
	for (uint8_t i = 0; i < 24; i++)
		write_leddrv(0xF + i, 0);
}

void test_leds()
{
	for (uint8_t el = 0; el < 24; el++) {

		// fade on next element
		for (uint8_t intensity = 0; intensity < 0xFF; intensity++) {
			write_leddrv(0xF + el, intensity);
			HAL_Delay(1);
		}

		write_leddrv(0xF + el, 0);
	}
	// fade all lights up
	for (uint8_t intensity = 0; intensity < 0xFF; intensity++) {
		for (uint8_t i = 0; i < 24; i++) {
			write_leddrv(0xF + i, intensity);
		}
		HAL_Delay(1);
	}

	// all lights off
	for (uint8_t i = 0; i < 24; i++)
		write_leddrv(0xF + i, 0);
}

//
// MUX IO
//

void set_mux_select(uint8_t sel)
{
	HAL_GPIO_WritePin(SEL0_GPIO_Port, SEL0_Pin, (sel & 0b001));
	HAL_GPIO_WritePin(SEL1_GPIO_Port, SEL1_Pin, (sel & 0b010));
	HAL_GPIO_WritePin(SEL2_GPIO_Port, SEL2_Pin, (sel & 0b100));
}

void write_to_mux(uint8_t val)
{
	for (unsigned chan = 0; chan < 8; chan++) {
		set_mux_select(chan);
		HAL_GPIO_WritePin(MUXWRITE_GPIO_Port, MUXWRITE_Pin, (val & (1 << chan)));
		HAL_Delay(1);
	}
}

uint16_t read_from_muxes()
{
	uint16_t read = 0;
	for (unsigned chan = 0; chan < 8; chan++) {
		set_mux_select(chan);
		// HAL_Delay(1);

		GPIO_PinState r = HAL_GPIO_ReadPin(MUXREAD0_GPIO_Port, MUXREAD0_Pin);
		if (r == GPIO_PIN_SET)
			read |= (1 << chan);

		r = HAL_GPIO_ReadPin(MUXREAD1_GPIO_Port, MUXREAD1_Pin);
		if (r == GPIO_PIN_SET)
			read |= (1 << chan);

		// HAL_Delay(1);
	}
	return read;
}

uint16_t read_write_to_mux(uint8_t val)
{
	uint16_t read = 0;
	for (unsigned chan = 0; chan < 8; chan++) {
		HAL_GPIO_WritePin(MUXWRITE_GPIO_Port, MUXWRITE_Pin, 0);
		set_mux_select(chan);

		HAL_GPIO_WritePin(MUXWRITE_GPIO_Port, MUXWRITE_Pin, (val & (1 << chan)));

		// HAL_Delay(1);

		if (HAL_GPIO_ReadPin(MUXREAD1_GPIO_Port, MUXREAD1_Pin))
			read |= (1 << chan);

		if (HAL_GPIO_ReadPin(MUXREAD0_GPIO_Port, MUXREAD0_Pin))
			read |= (1 << chan) << 8U;
	}
	return read;
}

void test_tmux_timing()
{
	while (1) {
		GPIO_PinState r = read_write_to_mux(0) > 0 ? GPIO_PIN_SET : GPIO_PIN_RESET;
		HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, r);
	}
}

void test_tmux_read()
{
	uint16_t buttons1 = 0;
	while (1) {
		buttons1 = read_from_muxes();

		for (uint8_t led = 0; led < 8; led++) {
			uint8_t mux0_button_pressed = buttons1 & (1 << led) ? 100 : 0;
			uint8_t mux1_button_pressed = buttons1 & (1 << (led + 8)) ? 100 : 0;

			write_leddrv(0xF + led * 3, mux0_button_pressed);
			write_leddrv(0xF + led * 3 + 1, mux1_button_pressed);
		}

		HAL_Delay(100);
	}
}

void test_tmux_readwrite()
{
	uint8_t light = 0;
	while (1) {
		uint16_t buttons = read_write_to_mux(light);
		light = buttons & 0xFF;
	}
	// all off
	write_to_mux(0);
}

//
// DAC
//

void write_dac(uint8_t data[3])
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, (uint8_t *)data, 3, 0xFFFF);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}

void test_dac()
{
	uint8_t data[3];
	data[0] = 0b01000000; // all channels power up
	data[1] = 0;
	data[2] = 0;
	write_dac(data);

	uint8_t pressed = 0;
	data[0] = 0b00110000; // Write and update channel 0
	data[1] = 128;
	data[2] = 0x55;
	while (1) {
		write_dac(data);
		data[1] += 1;

		uint16_t r = read_from_muxes() & 0x0001; // Alt button
		if (r) {
			if (!pressed) {
				pressed = 1;
				data[0] = 0b00110000 | ((data[0] + 1) & 7);
			}
		} else
			pressed = 0;
	}
}

//
// ADC
//

static uint16_t adcbuff[2];
void init_adc() { HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adcbuff, 2); }
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	// runs at about 283kHz on p2 with -O3
	//  Noise can be around 40/4096 (5.5bits)
	//  if we oversample by 16 (+4 bits), we get 17kHz sample rate
	//  if we oversample by 64 (+6 bits) we get 4kHz sample rate
	// HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_SET);
	// HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, GPIO_PIN_RESET);
}

void test_dac_adc()
{
	uint8_t data[3];
	data[0] = 0b01000000; // all channels power up
	data[1] = 0;
	data[2] = 0;
	write_dac(data);

	uint8_t pressed = 0;
	data[0] = 0b00110000; // Write and update channel 0
	data[1] = 128;
	data[2] = 0x55;
	while (1) {
		write_dac(data);
		// data[1] += 1;
		data[2] = adcbuff[1] & 0xFF;
		data[1] = adcbuff[1] >> 8;

		uint16_t r = read_from_muxes() & 0x0001; // Alt button
		if (r) {
			if (!pressed) {
				pressed = 1;
				data[0] = 0b00110000 | ((data[0] + 1) & 7);
			}
		} else
			pressed = 0;
	}
}

void test_dac_adc_leds()
{
	uint8_t data[3];
	data[0] = 0b01000000; // all channels power up
	data[1] = 0;
	data[2] = 0;
	write_dac(data);

	uint8_t pressed = 0;
	data[0] = 0b00110000; // Write and update channel 0
	data[1] = 128;
	data[2] = 0x55;

	uint8_t intensity = 0;
	uint8_t el = 0;
	while (1) {
		write_dac(data);
		// data[1] += 1;
		data[2] = adcbuff[1] & 0xFF;
		data[1] = adcbuff[1] >> 8;

		uint16_t r = read_from_muxes() & 0b0011; // Alt button or ? button
		if (r == 0b0001) {
			if (!pressed) {
				pressed = 1;
				data[0] = 0b00110000 | ((data[0] + 1) & 7);
			}
		} else
			pressed = 0;

		// fade LEDs randomly

		if (r != 0b0010) {
			write_leddrv(0xF + el, intensity);
			intensity += adcbuff[1] * 0xA7;
			if (++el >= 24)
				el = 0;
		}
	}
}

// Trig inputs
void test_trig_in()
{
	do {
		GPIO_PinState r = HAL_GPIO_ReadPin(TrigIN_GPIO_Port, TrigIN_Pin);
		HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, r);
	} while ((read_from_muxes() & 0x0001) == 0);
	while ((read_from_muxes() & 0x0001) == 1)
		;
}
void test_reset_in()
{
	do {
		GPIO_PinState r = HAL_GPIO_ReadPin(ResetJack_GPIO_Port, ResetJack_Pin);
		HAL_GPIO_WritePin(DEBUG1_GPIO_Port, DEBUG1_Pin, r);
	} while ((read_from_muxes() & 0x0001) == 0);
	while ((read_from_muxes() & 0x0001) == 1)
		;
}

int main(void)
{
	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();
	MX_DMA_Init();
	MX_ADC1_Init();
	MX_SPI1_Init();
	MX_I2C1_Init();
  MX_USART2_UART_Init();
	init_adc(); // on p2: 400kHz
	init_leds();

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = enc8a_Pin | enc8b_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(enc8b_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = enc8a_Pin | enc8b_Pin;
	HAL_GPIO_Init(enc8b_GPIO_Port, &GPIO_InitStruct);

	// test_trig_in();

	// test_reset_in();
	test_leds();
	// test_tmux_read();
	test_tmux_readwrite();
	// test_tmux_timing(); // on p2: max 200uS
	// test_dac();
	// test_dac_adc();
	// test_dac_adc_leds();

	while (1) {
	}
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig = {0};

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Configure the global features of the ADC (Clock, Resolution, Data
	 * Alignment and number of conversion)
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = ENABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_LEFT;
	hadc1.Init.NbrOfConversion = 2;
	hadc1.Init.DMAContinuousRequests = ENABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_SEQ_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	/** Configure for the selected ADC regular channel its corresponding rank in
	 * the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}

	sConfig.Channel = ADC_CHANNEL_1;
	sConfig.Rank = 2;
	sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

	/* USER CODE BEGIN SPI1_Init 0 */
	// GPIO_InitTypeDef GPIO_InitStruct;
	// GPIO_InitStruct.Pin = GPIO_PIN_4;
	// GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	// GPIO_InitStruct.Pull = GPIO_NOPULL;
	// GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	// HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	// for (int i = 0; i < 100000; i++) {
	// 	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 1);
	// 	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 0);
	// }

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_1LINE;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_HalfDuplex_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
 * Enable DMA controller clock
 */
static void MX_DMA_Init(void)
{

	/* DMA controller clock enable */
	__HAL_RCC_DMA2_CLK_ENABLE();

	/* DMA interrupt init */
	/* DMA2_Stream0_IRQn interrupt configuration */
	// HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
	// HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, DEBUG1_Pin | MUXWRITE_Pin | SEL0_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, SEL1_Pin | SEL2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : ResetJack_Pin TrigIN_Pin enc1b_Pin */
	GPIO_InitStruct.Pin = ResetJack_Pin | TrigIN_Pin | enc1b_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : DEBUG1_Pin MUXWRITE_Pin SEL0_Pin */
	GPIO_InitStruct.Pin = DEBUG1_Pin | MUXWRITE_Pin | SEL0_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins : SEL1_Pin SEL2_Pin */
	GPIO_InitStruct.Pin = SEL1_Pin | SEL2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : MUXREAD0_Pin MUXREAD1_Pin enc8a_Pin enc8b_Pin
							 enc2a_Pin enc1a_Pin enc4b_Pin enc4a_Pin
							 enc3a_Pin enc3B_Pin enc2b_Pin */
	GPIO_InitStruct.Pin = MUXREAD0_Pin | MUXREAD1_Pin | enc8a_Pin | enc8b_Pin | enc2a_Pin | enc1a_Pin | enc4b_Pin |
						  enc4a_Pin | enc3a_Pin | enc3B_Pin | enc2b_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : enc7b_Pin enc7a_Pin enc6a_Pin enc6b_Pin
							 enc5b_Pin enc5a_Pin */
	GPIO_InitStruct.Pin = enc7b_Pin | enc7a_Pin | enc6a_Pin | enc6b_Pin | enc5b_Pin | enc5a_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state
	 */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line
	   number, eg: printf("Wrong parameters value: file %s on line %d\r\n",
	   file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
