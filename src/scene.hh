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
	static constexpr type Range = Max - Min;

	static constexpr uint16_t from_volts(const float volts)
	{
		auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
		return MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, Min, Max);
	}

	static constexpr type inc_step = (Range / Model::output_octave_range / 12.f) + .5f;
	static constexpr type inc_step_fine = (Range / Model::output_octave_range / 12.f / 25.f) + .5f;
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

	// i think i will remove this..
	void adjust_chan(unsigned scene, unsigned chan, int32_t by)
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

	void inc_chan(unsigned scene, unsigned chan, bool fine = false)
	{
		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return;

		auto inc = ChannelValue::inc_step;

		if (fine)
			inc = ChannelValue::inc_step_fine;

		auto &out = banks[cur_bank].scene[scene].chans[chan];

		if (ChannelValue::Max - out <= inc)
			out = ChannelValue::Max;
		else
			out += inc;
	}

	void dec_chan(unsigned scene, unsigned chan, bool fine = false)
	{
		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return;

		auto inc = ChannelValue::inc_step;

		if (fine)
			inc = ChannelValue::inc_step_fine;

		auto &out = banks[cur_bank].scene[scene].chans[chan];

		if (out <= inc)
			out = ChannelValue::Min;
		else
			out -= inc;
	}

	void adj_chan(unsigned scene, unsigned chan, int dir, bool fine = false)
	{
		if (dir > 0)
			inc_chan(scene, chan, fine);
		else if (dir < 0)
			dec_chan(scene, chan, fine);
	}

private:
	using BankArray = std::array<Bank, Model::NumBanks>;
	BankArray banks;
	uint8_t cur_bank{0};
};

} // namespace Catalyst2
