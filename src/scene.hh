#pragma once
#include "conf/model.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Scene {
	using ChannelValue_t = uint16_t;

	enum class ChannelType : bool { CV, Gate };

	static constexpr ChannelValue_t gate_high = Model::volts_to_uint(5.f);

	std::array<ChannelValue_t, Model::NumChans> chans;
	std::array<ChannelType, Model::NumChans> types;

	Scene()
	{
		for (auto &c : chans) {
			c = 0;
		}
		for (auto &t : types) {
			t = ChannelType::CV;
		}
	}
};

struct Bank {
	using Scenes = std::array<Scene, Model::NumChans>;
	Scenes scene;
};

struct Part {
	using Banks = std::array<Bank, Model::NumBanks>;
	Banks bank;
	uint8_t cur_bank{0};

	void sel_bank(unsigned bank)
	{
		if (bank >= Model::NumBanks)
			return;

		cur_bank = bank;
	}
	void set_chan(unsigned scene, unsigned chan, Scene::ChannelValue_t val)
	{
		if (chan >= Model::NumChans || scene >= Model::NumChans)
			return;

		bank[cur_bank].scene[scene].chans[chan] = val;
	}
	Scene::ChannelValue_t get_chan(unsigned scene, unsigned chan)
	{
		if (chan >= Model::NumChans || scene >= Model::NumChans)
			return 0;

		return bank[cur_bank].scene[scene].chans[chan];
	}
	void inc_chan(unsigned scene, unsigned chan, int by)
	{
		if (chan >= Model::NumChans || scene >= Model::NumChans)
			return;

		if (bank[cur_bank].scene[scene].types[chan] == Scene::ChannelType::CV) {
			int temp = bank[cur_bank].scene[scene].chans[chan];
			temp += by;
			temp = std::clamp(temp, 0, 65535);
			bank[cur_bank].scene[scene].chans[chan] = static_cast<Scene::ChannelValue_t>(temp);
		}
	}
};

} // namespace Catalyst2
