#pragma once
#include "channelvalue.hh"
#include "conf/model.hh"
#include "pathway.hh"
#include "randompool.hh"
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>

namespace Catalyst2
{

class Banks {
	using ChannelArray = std::array<Channel, Model::NumChans>;
	struct Scene {
		ChannelArray channel;
		float random_amount = 0.f;
	};
	using SceneArray = std::array<Scene, Model::NumScenes>;
	struct Bank {
		SceneArray scene;
		Pathway path;
		std::bitset<Model::NumChans> isgate;
		std::bitset<Model::NumChans> isQuantized;
	};
	using BankArray = std::array<Bank, Model::NumBanks>;
	struct Clipboard {
		uint8_t scene : 3;
		uint8_t bank : 3;
	};

	// member variables
	BankArray bank;
	Clipboard clipboard = {.scene = 0, .bank = 0};
	uint8_t cur_bank = 0;

public:
	Pathway &Path()
	{
		return bank[cur_bank].path;
	}

	void CopySceneToClipboard(uint8_t scene)
	{
		clipboard.bank = cur_bank;
		clipboard.scene = scene;
	}

	void PasteToScene(uint8_t scene)
	{
		bank[cur_bank].scene[scene] = bank[clipboard.bank].scene[clipboard.scene];
	}

	float GetRandomAmount(uint8_t scene)
	{
		if (scene >= Model::NumScenes)
			return 0.f;

		return bank[cur_bank].scene[scene].random_amount;
	}

	void SetRandomAmount(uint8_t scene, float amount)
	{
		bank[cur_bank].scene[scene].random_amount = std::clamp(amount, 0.f, 1.f);
	}

	bool IsChanTypeGate(uint8_t channel)
	{
		if (channel >= Model::NumChans)
			return false;

		return bank[cur_bank].isgate[channel];
	}

	bool IsChanQuantized(uint8_t channel)
	{
		if (channel >= Model::NumChans)
			return false;

		return bank[cur_bank].isQuantized[channel];
	}

	ChannelValue::type GetChannel(uint8_t scene, uint8_t channel)
	{
		if (channel >= Model::NumChans || scene >= Model::NumScenes)
			return 0;

		auto rand = static_cast<int32_t>(RandomPool::GetRandomVal(cur_bank, scene, channel) *
										 bank[cur_bank].scene[scene].random_amount * ChannelValue::Range);

		if (bank[cur_bank].isgate[channel]) {
			// gates not affected by randomness?
			rand = 0;
		}
		auto temp = bank[cur_bank].scene[scene].channel[channel].val + rand;
		return std::clamp<int32_t>(temp, ChannelValue::Min, ChannelValue::Max);
	}

	void IncChan(uint8_t scene, uint8_t channel, int32_t dir, bool fine)
	{
		if (channel >= Model::NumChans || scene >= Model::NumScenes)
			return;
		bank[cur_bank].scene[scene].channel[channel].Inc(dir, fine, IsChanTypeGate(channel));
	}

	uint8_t GetSelBank()
	{
		return cur_bank;
	}

	void SelBank(uint8_t bank)
	{
		if (bank >= Model::NumBanks)
			return;

		cur_bank = bank;
	}
};

} // namespace Catalyst2
