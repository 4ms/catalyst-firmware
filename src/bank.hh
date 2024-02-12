#pragma once

#include "channel.hh"
#include "channelmode.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "macro_value.hh"
#include "pathway.hh"
#include "random.hh"
#include <algorithm>
#include <array>

namespace Catalyst2::Macro::Bank
{

struct BankData {
	struct Scene {
		std::array<Macro::Value, Model::NumChans> channel{};
		Random::Amount::type random_amount;
	};
	std::array<Scene, Model::NumScenes> scene{};
	std::array<Channel::Mode, Model::NumChans> channelmode{};
	std::array<Channel::Cv::Range, Model::NumChans> range{};
	std::array<float, Model::NumChans> morph{};
	BankData() {
		for (auto &m : morph) {
			m = 1.f;
		}
	}
	bool Validate() const {
		auto ret = true;
		for (auto &s : scene) {
			for (auto &cv : s.channel) {
				ret &= cv.Validate();
			}
			ret &= s.random_amount <= Random::Amount::max && s.random_amount >= Random::Amount::min;
		}
		for (auto &cm : channelmode) {
			ret &= cm.Validate();
		}
		for (auto &r : range) {
			ret &= r.Validate();
		}
		for (auto &m : morph) {
			ret &= (m >= 0.f && m <= 1.f);
		}
		return ret;
	}
};

struct Data {
	std::array<BankData, Model::TotalBanks> bank{};
	Pathway::Data pathway{};
	Random::Macro::Pool::Data randompool{};
	uint8_t cur_bank;

	bool Validate() const {
		auto ret = true;
		for (auto &b : bank) {
			ret &= b.Validate();
		}
		ret &= pathway.Validate();
		ret &= randompool.Validate();
		ret &= cur_bank < Model::TotalBanks;
		return ret;
	}
};

class Interface {
	Data &data;
	BankData::Scene clipboard{};

public:
	Pathway::Interface pathway{};
	Random::Macro::Pool::Interface randompool{data.randompool};

	Interface(Data &data)
		: data{data} {
	}
	void Clear() {
		data.bank[data.cur_bank] = BankData{};
		data.pathway.Clear(data.cur_bank);
	}
	void ClearScene(uint8_t scene) {
		data.bank[data.cur_bank].scene[scene] = BankData::Scene{};
	}
	void SelectBank(uint8_t bank) {
		data.cur_bank = bank < Model::TotalBanks ? bank : data.cur_bank;
		pathway.Load(data.pathway[data.cur_bank]);
	}
	uint8_t GetSelectedBank() const {
		return data.cur_bank;
	}
	bool IsBankClassic() const {
		return data.cur_bank == Model::TotalBanks - 1;
	}
	void Copy(uint8_t scene) {
		clipboard = data.bank[data.cur_bank].scene[scene];
	}
	void Paste(uint8_t scene) {
		data.bank[data.cur_bank].scene[scene] = clipboard;
	}
	void IncChannelMode(uint8_t channel, int32_t inc) {
		data.bank[data.cur_bank].channelmode[channel].Inc(inc);
	}
	void IncRange(uint8_t channel, int32_t inc) {
		data.bank[data.cur_bank].range[channel].Inc(inc);
	}
	Channel::Cv::Range GetRange(uint8_t channel) {
		return data.bank[data.cur_bank].range[channel];
	}
	Channel::Mode GetChannelMode(uint8_t channel) {
		return data.bank[data.cur_bank].channelmode[channel];
	}
	void SetChanMode(uint8_t channel, Channel::Mode mode) {
		data.bank[data.cur_bank].channelmode[channel] = mode;
	}
	auto GetRandomAmount(uint8_t scene) {
		return data.bank[data.cur_bank].scene[scene].random_amount;
	}
	void IncRandomAmount(uint8_t scene, int32_t inc) {
		auto t = data.bank[data.cur_bank].scene[scene].random_amount;
		t += inc * Random::Amount::inc;
		data.bank[data.cur_bank].scene[scene].random_amount = std::clamp(t, Random::Amount::min, Random::Amount::max);
	}
	void IncChan(uint8_t scene, uint8_t channel, int32_t inc, bool fine) {
		if (GetChannelMode(channel).IsGate()) {
			data.bank[data.cur_bank].scene[scene].channel[channel].IncGate(inc);
		} else {
			data.bank[data.cur_bank].scene[scene].channel[channel].IncCv(inc, fine, GetRange(channel));
		}
	}
	float GetMorph(uint8_t channel) {
		return 1.f - data.bank[data.cur_bank].morph[channel];
	}
	void IncMorph(uint8_t channel, int32_t inc) {
		const auto i = (1.f / 100.f) * inc;
		data.bank[data.cur_bank].morph[channel] = std::clamp(data.bank[data.cur_bank].morph[channel] + i, 0.f, 1.f);
	}
	Channel::Cv::type GetCv(uint8_t scene, uint8_t channel) {
		const auto rand = randompool.Read(channel, scene, data.bank[data.cur_bank].scene[scene].random_amount);
		return data.bank[data.cur_bank].scene[scene].channel[channel].ReadCv(rand);
	}
	Channel::Gate::type GetGate(uint8_t scene, uint8_t channel) {
		const auto rand = randompool.Read(channel, scene, data.bank[data.cur_bank].scene[scene].random_amount);
		return data.bank[data.cur_bank].scene[scene].channel[channel].ReadGate(rand);
	}
};

} // namespace Catalyst2::Macro::Bank
