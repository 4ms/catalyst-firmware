#pragma once

#include "channel.hh"
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

class Pool {
	// same pool of random values can be shared with the sequencer and the macro mode
	static constexpr auto size_macro = Model::NumScenes * Model::NumChans;
	static constexpr auto size_seq = Model::MaxSeqSteps * Model::NumChans;
	static constexpr auto size = size_macro > size_seq ? size_macro : size_seq;
	std::array<int8_t, size> val;

public:
	// not really the seed but works well enough as such
	uint8_t GetSeed() {
		return val[0] + 128;
	}
	void ClearScene() {
		std::fill(val.begin(), val.data() + size_macro, 0);
	}
	void ClearSequence() {
		std::fill(val.begin(), val.data() + size_seq, 0);
	}
	void RandomizeScene() {
		for (auto i = 0u; i < size_macro; i++) {
			Randomize(i);
		}
		if (val[0] == 0) {
			val[0] = 1;
		}
	}
	void RandomizeSequence() {
		for (auto i = 0u; i < size_seq; i++) {
			Randomize(i);
		}
		if (val[0] == 0) {
			val[0] = 1;
		}
	}
	bool IsRandomized() {
		return val[0] != 0;
	}

	int32_t GetMacroOffset(uint8_t scene, uint8_t chan, Amount amnt, Channel::Range range) {
		auto t = GetSceneVal(scene, chan) * amnt.Read();
		if (t < 0.f) {
			t *= range.NegAmount();
		} else {
			t *= range.PosAmount();
		}
		return t * Channel::range;
	}
	int32_t GetSequenceOffset(uint8_t sequence, uint8_t step, Amount amnt, Channel::Range range) {
		auto t = GetSequenceVal(sequence, step) * amnt.Read();
		if (t < 0.f) {
			t *= range.NegAmount();
		} else {
			t *= range.PosAmount();
		}
		return t * Channel::range;
	}

private:
	float GetSceneVal(uint8_t scene, uint8_t chan) {
		return Read((scene * Model::NumScenes) + chan);
	}
	float GetSequenceVal(uint8_t sequence, uint8_t step) {
		return Read((sequence * Model::MaxSeqSteps) + step);
	}
	float Read(uint32_t idx) {
		return val[idx] / 128.f;
	}
	void Randomize(uint8_t idx) {
		val[idx] = std::rand();
	}
};
} // namespace Catalyst2::Random