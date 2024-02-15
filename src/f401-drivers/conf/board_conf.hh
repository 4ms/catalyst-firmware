#pragma once

#include "conf/model.hh"
#include "drivers/adc_builtin_conf.hh"
#include "drivers/debounced_switch.hh"
#include "drivers/i2c_config_struct.hh"
#include "drivers/muxed_io.hh"
#include "drivers/rotary_general.hh"
#include "drivers/spi_config_struct.hh"
#include "drivers/timekeeper.hh"
#include <array>

// Configuration for everything that might change if we use a different ICs
namespace Catalyst2::Board
{

using GPIO = mdrivlib::GPIO;
using PinDef = mdrivlib::PinDef;
using PinAF = mdrivlib::PinAF;
using PinNum = mdrivlib::PinNum;
using PinMode = mdrivlib::PinMode;
using PinPolarity = mdrivlib::PinPolarity;
using TimekeeperConfig = mdrivlib::TimekeeperConfig;

//////////////// Trigger Inputs

using TrigJack = mdrivlib::DebouncedPin<PinDef{GPIO::C, PinNum::_14}, PinPolarity::Inverted>;
using ResetJack = mdrivlib::DebouncedPin<PinDef{GPIO::C, PinNum::_13}, PinPolarity::Inverted>;

//////////////// Buttons

struct Buttons {
	static constexpr std::array<uint8_t, Model::NumChans> SceneMap{
		11,
		8,
		7,
		5,
		9,
		10,
		4,
		6,
	};

	static constexpr uint8_t Shift = 1;
	static constexpr uint8_t Morph = 12;
	static constexpr uint8_t Bank = 2;
	static constexpr uint8_t Fine = 14;
	static constexpr uint8_t Add = 0;
	static constexpr uint8_t Play = 15;
	static constexpr uint8_t ClockJackSense = 13;
	static constexpr uint8_t UnusedMuxInput = 3;
};

//////////////// Encoders

inline constexpr mdrivlib::RotaryStepSize EncStepSize = mdrivlib::RotaryFullStep;
inline constexpr PinDef Enc1A{GPIO::B, PinNum::_7};
inline constexpr PinDef Enc1B{GPIO::C, PinNum::_15};
inline constexpr PinDef Enc2A{GPIO::B, PinNum::_6};
inline constexpr PinDef Enc2B{GPIO::B, PinNum::_5};
inline constexpr PinDef Enc3A{GPIO::B, PinNum::_3};
inline constexpr PinDef Enc3B{GPIO::B, PinNum::_4};
inline constexpr PinDef Enc4A{GPIO::A, PinNum::_15};
inline constexpr PinDef Enc4B{GPIO::A, PinNum::_12};
inline constexpr PinDef Enc5A{GPIO::A, PinNum::_11};
inline constexpr PinDef Enc5B{GPIO::A, PinNum::_10};
inline constexpr PinDef Enc6A{GPIO::A, PinNum::_8};
inline constexpr PinDef Enc6B{GPIO::A, PinNum::_9};
inline constexpr PinDef Enc7A{GPIO::B, PinNum::_15};
inline constexpr PinDef Enc7B{GPIO::B, PinNum::_14};
inline constexpr PinDef Enc8A{GPIO::B, PinNum::_12};
inline constexpr PinDef Enc8B{GPIO::B, PinNum::_13};

//////////////// Muxes

inline constexpr unsigned NumInputMuxChips = 2;
inline constexpr unsigned NumOutputMuxChips = 1;
inline constexpr std::array<mdrivlib::PinDef, NumInputMuxChips> MuxInputChipPins{{
	{GPIO::B, PinNum::_2},
	{GPIO::B, PinNum::_10},
}};
inline constexpr std::array<mdrivlib::PinDef, NumOutputMuxChips> MuxOutputChipPins{{
	{GPIO::A, PinNum::_3},
}};
inline constexpr std::array<mdrivlib::PinDef, 3> MuxSelectPins{{
	{GPIO::A, PinNum::_6},
	{GPIO::B, PinNum::_0},
	{GPIO::B, PinNum::_1},
}};
inline constexpr MuxedIOConfig<Board::NumInputMuxChips, Board::NumOutputMuxChips> MuxConf{
	.InputChipPins = {{
		{GPIO::B, PinNum::_2},
		{GPIO::B, PinNum::_10},
	}},
	.OutputChipPins = Board::MuxOutputChipPins,
	.SelectPins = Board::MuxSelectPins,
};

inline constexpr std::array<uint32_t, Model::NumChans> ButtonLedMap{4, 6, 2, 1, 7, 5, 3, 0};

using PlayLed = mdrivlib::FPin<GPIO::A, PinNum::_2, PinMode::Output>;

//////////////// LED Driver

const mdrivlib::I2CConfig LedDriverConf{
	.I2Cx = I2C1,
	.SCL = {GPIO::B, PinNum::_8, PinAF::AltFunc4},
	.SDA = {GPIO::B, PinNum::_9, PinAF::AltFunc4},
	.timing = {400'000},
};

inline constexpr uint8_t LedDriverAddr = 0b0101'0000;

inline constexpr std::array<uint32_t, Model::NumChans> EncLedMap{4, 5, 6, 7, 0, 1, 2, 3};

//////////////// ADC

struct AdcConf : mdrivlib::DefaultAdcPeriphConf {
	static constexpr auto resolution = mdrivlib::AdcResolution::Bits12;
	static constexpr auto adc_periph_num = mdrivlib::AdcPeriphNum::_1;
	static constexpr auto oversample = false;
	static constexpr auto clock_div = mdrivlib::AdcClockSourceDiv::APBClk_Div2;

	static constexpr bool enable_end_of_sequence_isr = true;
	static constexpr bool enable_end_of_conversion_isr = false;

	struct DmaConf : mdrivlib::DefaultAdcPeriphConf::DmaConf {
		static constexpr auto DMAx = 2;
		static constexpr auto StreamNum = 0;
		static constexpr auto RequestNum = DMA_CHANNEL_0;
		static constexpr auto dma_priority = Low;
		static constexpr IRQn_Type IRQn = DMA2_Stream0_IRQn;
		static constexpr uint32_t pri = 3;
		static constexpr uint32_t subpri = 3;
	};

	static constexpr uint16_t uni_min_value = 20;
};

inline constexpr auto NumAdcs = 2u;
inline constexpr std::array<mdrivlib::AdcChannelConf, NumAdcs> AdcChans = {{
	{{GPIO::A, PinNum::_0}, mdrivlib::AdcChanNum::_0, 0, mdrivlib::AdcSamplingTime::_56Cycles},
	{{GPIO::A, PinNum::_1}, mdrivlib::AdcChanNum::_1, 1, mdrivlib::AdcSamplingTime::_56Cycles},
}};

////////////////// DAC

struct DacSpiConf : mdrivlib::DefaultSpiConf {
	static constexpr uint16_t PeriphNum = 1; // 1 ==> SPI1, 2 ==> SPI2, etc
	static constexpr PinDef SCLK = {GPIO::A, PinNum::_5, PinAF::AltFunc5};
	static constexpr PinDef COPI = {GPIO::A, PinNum::_7, PinAF::AltFunc5};
	static constexpr PinDef CS0 = {GPIO::A, PinNum::_4, PinAF::AFNone};
	static constexpr bool use_hardware_ss = false;
	static constexpr uint16_t clock_division = 2;
	static constexpr uint16_t data_size = 8;
	static constexpr auto data_dir = mdrivlib::SpiDataDir::TXOnly;

	// DAC chip: First falling edge of SCK after CS goes low captures SDI
	static constexpr bool clock_high_when_idle = true;
	static constexpr bool second_clk_transition_captures_data = false;
};

////////////////// Stream "thread" configuration
const TimekeeperConfig cv_stream_conf{
	.TIMx = TIM3,
	.period_ns = TimekeeperConfig::Hz(Model::sample_rate_hz),
	.priority1 = 2,
	.priority2 = 2,
};

//@10Hz clock
// old way: step jitter is 1.5ms. PW jitter is 620uS
// new way: step jitter is 3.8ms. PW jitter is 200us
//@2Hz clock
// old way: step jitter is 5.5ms. PW jiiter the same
// new way: step jitter is 20ms!
inline constexpr unsigned encoder_led_hz = 60;
const TimekeeperConfig encoder_led_task{
	.TIMx = TIM2,
	.period_ns = TimekeeperConfig::Hz(encoder_led_hz * Model::NumChans),
	.priority1 = 1,
	.priority2 = 3,
};

inline constexpr unsigned muxio_update_hz = 16000;
const TimekeeperConfig muxio_conf{
	.TIMx = TIM5,
	.period_ns = TimekeeperConfig::Hz(muxio_update_hz),
	.priority1 = 0,
	.priority2 = 2,
};

///////////////// Debug pin

using DebugPin = mdrivlib::FPin<GPIO::A, PinNum::_2, PinMode::Output>;

} // namespace Catalyst2::Board
