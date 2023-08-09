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
	// DG: I could see an 8-output device having more than 8 scenes per bank
	//  we should change NumChans=>NumScenes or NumScenesPerBank
	using Scenes = std::array<Scene, Model::NumChans>;
	Scenes scene;
};

// DG: this is a great class, what does the name refer to?
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
			// DG: int != ChannelValue_t
			int temp = bank[cur_bank].scene[scene].chans[chan];
			temp += by;
			// DG: These values depend on the what ChannelValue_t is, so somehow it should not be raw numbers. Maybe
			// ChannelValueMax/Min?
			temp = std::clamp(temp, 0, 65535);
			// Don't need to static_cast, its already a uint16_t
			bank[cur_bank].scene[scene].chans[chan] = static_cast<Scene::ChannelValue_t>(temp);
		}
	}
};

} // namespace Catalyst2
