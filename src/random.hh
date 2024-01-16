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

template<typename T>
void RandomizeBuffer(T &d) {
	for (auto &r : d) {
		r = std::rand();
	}
}
namespace Pool
{
using type = float;

struct SeqData : public std::array<int8_t, Model::MaxSeqSteps * Model::NumChans> {
	SeqData() {
		RandomizeBuffer(*this);
	}
	bool Validate() const {
		return true;
	}
};

struct MacroData : public std::array<int8_t, Model::NumScenes * Model::NumChans> {
	MacroData() {
		RandomizeBuffer(*this);
	}
	bool Validate() const {
		return true;
	}
};

template<typename T>
class Interface {
	T &data;
	bool is_random = true;

public:
	Interface(T &data)
		: data{data} {
	}
	uint8_t GetSeed() const {
		return is_random ? data[0] + 128 : 0;
	}
	void Clear() {
		is_random = false;
	}
	void Randomize() {
		is_random = true;
		RandomizeBuffer(data);
	}
	bool IsRandomized() const {
		return is_random;
	}
	type Read(uint8_t channel, uint8_t step_or_scene, float amnt) const {
		if (!is_random) {
			return 0.f;
		}
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
namespace Steps
{
struct Data : public std::array<std::array<uint8_t, Model::MaxSeqSteps>, Model::NumChans> {
	Data() {
		for (auto &c : *this) {
			RandomizeBuffer(c);
		}
	}
	bool Validate() const {
		return true;
	}
};
class Interface {
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	void Randomize(uint8_t chan) {
		RandomizeBuffer(data[chan]);
	}
	void Randomize() {
		data = Data{};
	}
	uint8_t Read(uint8_t channel, uint8_t step) const {
		return data[channel][step];
	}
};
} // namespace Steps
} // namespace Catalyst2::Random