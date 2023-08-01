#pragma once
#include "drivers/adc_builtin_conf.hh"
#include "drivers/debounced_switch.hh"
#include "drivers/spi_config_struct.hh"
#include "drivers/timekeeper.hh"
#include <array>

namespace Catalyst2::Board
{
using GPIO = mdrivlib::GPIO;
using PinDef = mdrivlib::PinDef;
using PinAF = mdrivlib::PinAF;
using PinNum = mdrivlib::PinNum;
using PinMode = mdrivlib::PinMode;

enum class AdcElement { Slider, CVJack };
constexpr auto NumAdcs = 2;

using DebugPin = mdrivlib::FPin<GPIO::A, PinNum::_2, PinMode::Output>;

constexpr uint32_t NumOuts = 8;
using OutputBuffer = std::array<uint16_t, NumOuts>;

// using PingJack = mdrivlib::DebouncedPin<BrainPin::D6, Normal>;

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
		static constexpr uint32_t pri = 0;
		static constexpr uint32_t subpri = 0;
	};

	static constexpr uint16_t uni_min_value = 20;
};

constexpr std::array<mdrivlib::AdcChannelConf, NumAdcs> AdcChans = {{
	{{GPIO::A, PinNum::_0}, mdrivlib::AdcChanNum::_0, mdrivlib::AdcSamplingTime::_56Cycles},
	{{GPIO::A, PinNum::_1}, mdrivlib::AdcChanNum::_1, mdrivlib::AdcSamplingTime::_56Cycles},
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
};

constexpr mdrivlib::TimekeeperConfig cv_stream_conf{
	// TODO
};

} // namespace Catalyst2::Board
