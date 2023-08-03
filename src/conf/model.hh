#pragma once
#include <array>
#include <cstdint>

// User-facing features:
// Configuration for everything that might change if we make a version with more or less user-facing features
namespace Catalyst2::Model
{

constexpr uint32_t NumChans = 8;
constexpr uint32_t NumBanks = 8;

using OutputBuffer = std::array<uint16_t, NumChans>;

enum class ModeSwitch { Sequence, Macro };

enum class AdcElement { Slider, CVJack };

} // namespace Catalyst2::Model
