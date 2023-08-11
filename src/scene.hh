#pragma once
#include "conf/model.hh"
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct ChannelValue {
	using type = uint16_t;
	static constexpr type Max = UINT16_MAX;
	static constexpr type Min = 0;

	static constexpr uint16_t from_volts(const float volts)
	{
		auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
		return MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, Min, Max);
	}
};

struct Scene {

	enum class ChannelType : bool { CV, Gate };

	static constexpr ChannelValue::type gate_high = ChannelValue::from_volts(5.f);

	std::array<ChannelValue::type, Model::NumChans> chans;
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
	using Scenes = std::array<Scene, Model::NumScenes>;
	Scenes scene;
};

struct Banks {
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
	void set_chan(unsigned scene, unsigned chan, ChannelValue::type val)
	{
		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return;

		banks[cur_bank].scene[scene].chans[chan] = val;
	}
	ChannelValue::type get_chan(unsigned scene, unsigned chan)
	{
		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return 0;

		return banks[cur_bank].scene[scene].chans[chan];
	}
	void inc_chan(unsigned scene, unsigned chan, int32_t by)
	{
		if (by == 0)
			return;

		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return;

		if (by == INT32_MIN)
			by += 1;

		if (banks[cur_bank].scene[scene].types[chan] == Scene::ChannelType::CV) {
			auto temp = banks[cur_bank].scene[scene].chans[chan];

			if (by > 0 && ChannelValue::Max - temp < static_cast<ChannelValue::type>(by))
				temp = ChannelValue::Max;
			else if (by < 0 && by * -1 > temp)
				temp = ChannelValue::Min;
			else
				temp += by;

			banks[cur_bank].scene[scene].chans[chan] = temp;
		}
	}

private:
	using BankArray = std::array<Bank, Model::NumBanks>;
	BankArray banks;
	uint8_t cur_bank{0};
};

} // namespace Catalyst2
