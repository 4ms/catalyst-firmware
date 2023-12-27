#pragma once

#include "conf/model.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Random
{
class Amount {
	using type = uint8_t;
	static constexpr type min = 0, max = 31;
	type val = 0;

public:
	void Inc(int32_t inc) {
		val = std::clamp<int8_t>(val + inc, min, max);
	}
	float Read() {
		return val / static_cast<float>(max);
	}
	bool Validate() {
		return val >= min && val <= max;
	}
};

namespace Pool
{

using SeqData = std::array<int8_t, Model::MaxSeqSteps * Model::NumChans>;
using MacroData = std::array<int8_t, Model::NumScenes * Model::NumChans>;
using type = float;

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
		for (auto &r : data) {
			r = std::rand();
			r = r == 0 ? 1 : r;
		}
	}
	bool IsRandomized() const {
		return data[0] != 0;
	}
	type Read(uint8_t channel, uint8_t step_or_scene, Catalyst2::Random::Amount amnt) const {
		float t;
		if constexpr (std::same_as<T, SeqData>) {
			t = data[(channel * Model::MaxSeqSteps) + step_or_scene];
		} else if (std::same_as<T, MacroData>) {
			t = data[(channel * Model::NumChans) + step_or_scene];
		}
		t /= 128.f;
		return t * amnt.Read();
	}
};
} // namespace Pool
} // namespace Catalyst2::Random