#pragma once

#include "conf/model.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{
class RandomPool {
	// same pool of random values can be shared with the sequencer and the macro mode
	static constexpr auto size_macro = Model::NumScenes * Model::NumChans;
	static constexpr auto size_seq = Model::MaxSeqSteps * Model::NumChans;
	static constexpr auto size_ = size_macro > size_seq ? size_macro : size_seq;
	std::array<int8_t, size_> val;

public:
	// not really the seed but works well enough as such
	uint8_t GetSeed()
	{
		return val[0] + 128;
	}
	uint8_t GetSeedSequence(uint8_t sequence)
	{
		return val[sequence * Model::MaxSeqSteps] + 128;
	}
	void ClearScene()
	{
		std::fill(val.begin(), val.data() + size_macro, 0);
	}
	void RandomizeScene()
	{
		for (auto i = 0u; i < size_macro; i++)
			Randomize(i);
	}
	void ClearSequence(uint8_t sequence)
	{
		const auto offset = sequence * Model::MaxSeqSteps;

		std::fill(val.begin() + offset, val.data() + offset + Model::MaxSeqSteps, 0);
	}
	void ClearSequence()
	{
		std::fill(val.begin(), val.end(), 0);
	}
	void RandomizeSequence(uint8_t sequence)
	{
		const auto offset = sequence * Model::MaxSeqSteps;

		for (auto i = 0u; i < Model::MaxSeqSteps; i++)
			Randomize(i + offset);
	}
	void RandomizeSequence()
	{
		for (auto &v : val)
			v = std::rand();
	}
	float GetSceneVal(uint8_t scene, uint8_t chan)
	{
		return val[(scene * Model::NumScenes) + chan] / 128.f;
	}
	float GetSequenceVal(uint8_t sequence, uint8_t step)
	{
		const auto offset = sequence * Model::MaxSeqSteps;
		return val[offset + step] / 128.f;
	}

private:
	void Randomize(uint8_t idx)
	{
		val[idx] = std::rand();
	}
};
} // namespace Catalyst2