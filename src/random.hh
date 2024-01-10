#pragma once

#include "conf/model.hh"
#include "range.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace Catalyst2::Random
{
namespace Amount
{
using type = float;
inline constexpr auto min = 0.f, max = 1.f, def = 0.f, inc = (max / (Model::output_octave_range * 12)) * 2;
} // namespace Amount

namespace Pool
{

using type = float;

template<typename T>
void RandomizeBuffer(T &d) {
	for (auto &r : d) {
		r = std::rand();
		r = r == 0 ? 1 : r;
	}
}

struct SeqData : public std::array<int8_t, Model::MaxSeqSteps * Model::NumChans> {
	SeqData() {
		RandomizeBuffer(*this);
	}
};

struct MacroData : public std::array<int8_t, Model::NumScenes * Model::NumChans> {
	MacroData() {
		RandomizeBuffer(*this);
	}
};

template<typename T>
class Interface {
	T &data;

public:
	Interface(T &data)
		: data{data} {
	}
	uint8_t GetSeed() const {
		return data[0] + 128;
	}
	void Clear() {
		std::fill(data.begin(), data.end(), 0);
	}
	void Randomize() {
		RandomizeBuffer(data);
	}
	bool IsRandomized() const {
		return data[0] != 0;
	}
	type Read(uint8_t channel, uint8_t step_or_scene, float amnt) const {
		float t;
		if constexpr (std::same_as<T, SeqData>) {
			t = data[(channel * Model::MaxSeqSteps) + step_or_scene];
		} else if (std::same_as<T, MacroData>) {
			t = data[(channel * Model::NumChans) + step_or_scene];
		}
		t /= 128.f;
		return t * amnt;
	}
};
} // namespace Pool
} // namespace Catalyst2::Random