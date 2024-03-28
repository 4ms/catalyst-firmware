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

namespace Catalyst2::Legacy::V1_0::Macro::Bank
{

struct BankData {
	struct Scene {
		std::array<Macro::Value, Model::NumChans> channel{};
		Random::Amount::type random_amount = 0.f;
	};
	std::array<Scene, Model::Macro::NumScenes> scene{};
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
	std::array<BankData, Model::Macro::Bank::NumTotal> bank{};
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
		ret &= cur_bank < bank.size();
		return ret;
	}
};
} // namespace Catalyst2::Legacy::V1_0::Macro::Bank
