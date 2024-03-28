#pragma once

#include "conf/model.hh"
#include "sequencer_step.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <span>

namespace Catalyst2::Legacy::V1_0::Random
{
namespace Amount
{
using type = float;
inline constexpr auto min = 0.f;
inline constexpr auto max = 1.f;

// +/1 2 semitones
inline constexpr auto inc = (max / (Model::output_octave_range * 12));
inline constexpr auto def = inc;
} // namespace Amount
template<typename T>
inline void RandomizeBuffer(T &d) {
	for (auto &r : d) {
		r = std::rand();
	}
}

namespace Macro::Pool
{

using type = float;

struct Data : public std::array<int8_t, Model::Macro::NumScenes * Model::NumChans> {
	Data() {
		RandomizeBuffer(*this);
	}
	bool Validate() const {
		return true;
	}
};

} // namespace Macro::Pool
} // namespace Catalyst2::Legacy::V1_0::Random
