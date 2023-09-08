#pragma once
#include "conf/model.hh"
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>

namespace Catalyst2
{

struct ChannelValue {
	using type = int32_t;
	static constexpr type Max = UINT16_MAX;
	static constexpr type Min = 0;
	static constexpr type Range = Max - Min;
	static constexpr type Center = (Range / 2) + Min;

	static constexpr type from_volts(const float volts)
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
	std::array<int8_t, Model::NumChans> random_value;
	float random_amount;
	std::array<ChannelType, Model::NumChans> types;

	Scene()
	{
		init();
	}
	void init()
	{
		for (auto &c : chans) {
			c = ChannelValue::from_volts(2.f);
		}
		for (auto &t : types) {
			t = ChannelType::CV;
		}
		for (auto &rv : random_value) {
			rv = 0;
		}
		random_amount = 1.f / 15.f;
	}
};

struct Bank {
	using Scenes = std::array<Scene, Model::NumScenes>;
	Scenes scene;
};

class Banks {
	using BankArray = std::array<Bank, Model::NumBanks>;
	BankArray bank;
	uint8_t cur_bank{0};
	std::bitset<Model::NumBanks> edited;

public:
	// i dont know where to put this...
	// struct RangeSetting {
	// 	ChannelValue::type Min;
	// 	ChannelValue::type Max;
	// };

	// static constexpr RangeSetting full{ChannelValue::Min, ChannelValue::Max};
	// static constexpr RangeSetting bip5{ChannelValue::from_volts(-5.f), ChannelValue::from_volts(5.f)};
	// static constexpr RangeSetting pos5{ChannelValue::from_volts(0.f), ChannelValue::from_volts(5.f)};
	// static constexpr RangeSetting pos10{ChannelValue::from_volts(0.f), ChannelValue::from_volts(10.f)};

	// static constexpr std::array output_ranges = {
	// 	full,
	// 	bip5,
	// 	pos5,
	// 	pos10,
	// };

	void randomize()
	{
		auto &b = bank[cur_bank];
		for (auto &s : b.scene) {
			for (auto &c : s.random_value) {
				c = std::rand();
			}
		}
		edited[cur_bank] = true;
	}

	void clear_random()
	{
		for (auto &b : bank) {
			for (auto &s : b.scene) {
				for (auto &c : s.random_value) {
					c = 0;
				}
			}
		}
	}

	auto get_random_seed() const
	{
		return bank[0].scene[0].random_value[0];
	}

	float get_scene_random_amount(unsigned scene) const
	{
		return bank[cur_bank].scene[scene].random_amount;
	}

	void set_scene_random_amount(unsigned scene, float amount)
	{
		bank[cur_bank].scene[scene].random_amount = std::clamp(amount, 0.f, 1.f);
		edited[cur_bank] = true;
	}

	void sel_bank(unsigned bank)
	{
		if (bank >= Model::NumBanks)
			return;

		cur_bank = bank;
	}

	auto get_sel_bank() const
	{
		return cur_bank;
	}

	auto is_edited(unsigned bank_) const
	{
		if (bank_ >= Model::NumBanks)
			return false;

		return edited[bank_];
	}

	void reset_bank(unsigned bank_)
	{
		if (bank_ >= Model::NumBanks)
			return;

		for (auto &s : bank[bank_].scene) {
			s.init();
		}
		edited[bank_] = false;
	}

	ChannelValue::type get_chan(unsigned scene, unsigned chan) const
	{
		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return 0;

		auto &s = bank[cur_bank].scene[scene];
		auto temp = s.chans[chan];
		int r = ((s.random_value[chan] / 128.f) * s.random_amount) * (ChannelValue::Range / 2);
		if (r > 0) {
			if (ChannelValue::Max - temp <= r)
				temp = ChannelValue::Max;
			else
				temp += r;
		} else {
			r *= -1;
			if (temp <= r)
				temp = ChannelValue::Min;
			else
				temp -= r;
		}
		return temp;
	}

	void adj_chan(unsigned scene, unsigned chan, int dir, bool fine = false)
	{
		if (chan >= Model::NumChans || scene >= Model::NumScenes)
			return;

		auto inc = ChannelValue::inc_step;

		if (fine)
			inc = ChannelValue::inc_step_fine;

		auto &out = bank[cur_bank].scene[scene].chans[chan];

		if (dir > 0)
			out = inc_chan(out, inc);
		else if (dir < 0)
			out = dec_chan(out, inc);

		edited[cur_bank] = true;
	}

private:
	ChannelValue::type inc_chan(auto out, auto inc)
	{
		if (ChannelValue::Max - out <= inc)
			out = ChannelValue::Max;
		else
			out += inc;

		return out;
	}

	ChannelValue::type dec_chan(auto out, auto inc)
	{
		if (out <= inc)
			out = ChannelValue::Min;
		else
			out -= inc;

		return out;
	}
};

} // namespace Catalyst2
