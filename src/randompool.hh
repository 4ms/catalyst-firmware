#pragma once

#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{
class RandomPool {
	// same pool of random values can be shared with the sequencer and the macro mode
	static constexpr auto size_macro = Model::NumBanks * Model::NumScenes * Model::NumChans;
	static constexpr auto size_seq = Model::MaxSeqSteps * Model::NumChans;
	static constexpr auto size_ = size_macro > size_seq ? size_macro : size_seq;
	static inline std::array<int8_t, size_> val{};

	static constexpr auto bank_offset = Model::NumScenes * Model::NumChans;

public:
	static void Init(uint16_t seed)
	{
		std::srand(seed);
	}
	// not really the seed but works well enough as such
	static int8_t GetSeed(uint8_t bank)
	{
		if (bank >= Model::NumBanks)
			return 0;

		return val[bank_offset * bank];
	}
	static void ClearBank(uint8_t bank)
	{
		if (bank >= Model::NumBanks)
			return;
		const auto offset = bank * bank_offset;

		for (auto i = 0u; i < bank_offset; i++)
			val[i + offset] = 0;
	}
	static void RandomizeBank(uint8_t bank)
	{
		if (bank >= Model::NumBanks)
			return;
		const auto offset = bank * bank_offset;

		for (auto i = 0u; i < bank_offset; i++)
			Randomize(i + offset);
	}
	static void ClearSequenceChannel(uint8_t chan)
	{
		if (chan >= Model::NumChans)
			return;

		const auto offset = chan * Model::MaxSeqSteps;

		for (auto i = 0u; i < Model::MaxSeqSteps; i++)
			val[i + offset] = 0;
	}
	static void RandomizeSequenceChannel(uint8_t chan)
	{
		if (chan >= Model::NumChans)
			return;

		const auto offset = chan * Model::MaxSeqSteps;

		for (auto i = 0u; i < Model::MaxSeqSteps; i++)
			Randomize(i + offset);
	}
	static float GetRandomVal(uint8_t bank, uint8_t scene, uint8_t chan)
	{
		if (bank >= Model::NumBanks || scene >= Model::NumScenes || chan >= Model::NumChans)
			return 0;
		return val[bank * bank_offset + scene * Model::NumScenes + chan] / 128.f;
	}
	static uint8_t GetRandomVal(uint8_t seqchan, uint8_t step)
	{
		if (step >= Model::MaxSeqSteps || seqchan >= Model::NumChans)
			return 0;

		const auto offset = seqchan * Model::MaxSeqSteps;
		return val[offset + step];
	}

private:
	static void Randomize(uint8_t idx)
	{
		val[idx] = std::rand();
	}
};
} // namespace Catalyst2