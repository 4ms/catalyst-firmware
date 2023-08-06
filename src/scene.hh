#pragma once
#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Scene {
	using ChannelValue_t = uint16_t;

	enum class ChannelType : bool { CV, Gate };

	std::array<ChannelValue_t, Model::NumChans> chans;
	std::array<ChannelType, Model::NumChans> types;

	Scene()
	{
		for (auto &c : chans) {
			c = 0;
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
	std::array<uint8_t, 2> cur_scene{0, 4};
	uint8_t cur_bank{0};

	void sel_bank(unsigned bank)
	{
		if (bank >= Model::NumBanks)
			return;

		cur_bank = bank;
	}
	auto get_sel_bank()
	{
		return cur_bank;
	}

	void sel_scene(bool ab, unsigned scene)
	{
		if (scene >= Model::NumChans)
			return;

		cur_scene[ab] = scene;
	}
	auto get_sel_scene(bool ab)
	{
		return cur_scene[ab];
	}
	auto &get_scene(bool ab)
	{
		return bank[cur_bank].scene[cur_scene[ab]];
	}
	void set_chan(bool ab, unsigned chan, Scene::ChannelValue_t val)
	{
		if (chan >= Model::NumChans)
			return;

		bank[cur_bank].scene[cur_scene[ab]].chans[chan] = val;
	}
	Scene::ChannelValue_t get_chan(bool ab, unsigned chan)
	{
		if (chan >= Model::NumChans)
			return 0;

		return bank[cur_bank].scene[cur_scene[ab]].chans[chan];
	}
	void inc_chan(bool ab, int chan, int by)
	{
		int temp = get_chan(ab, chan);
		temp += by;
		if (temp < 0)
			temp = 0;
		if (temp >= 65536)
			temp = 65535;
		set_chan(ab, chan, temp);
	}
};

} // namespace Catalyst2
