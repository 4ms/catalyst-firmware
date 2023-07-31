#pragma once
#include "drivers/adc_builtin_conf.hh"
#include "drivers/debounced_switch.hh"
#include <array>

namespace Catalyst2::Board
{
enum class AdcElement { Slider, CVJack };

using DebugPin = FPin<mdrivlib::GPIO::A, mdrivlib::Pin::_1>;

static constexpr uint32_t NumOuts = 8;
using OutputBuffer = std::array<uint16_t, NumOuts>;

// using PingJack = mdrivlib::DebouncedPin<BrainPin::D6, Normal>;

// constexpr std::array<AdcChannelConf, NumPots> PotAdcChans = {{
// 	{BrainPin::A3, BrainPin::A3AdcChan, TimePot, Brain::AdcSampTime},
// 	{BrainPin::A5, BrainPin::A5AdcChan, FeedbackPot, Brain::AdcSampTime},
// }};

} // namespace Catalyst2::Board
