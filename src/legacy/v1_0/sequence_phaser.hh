#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include <array>

namespace Catalyst2::Legacy::V1_0::Sequencer::Phaser
{

struct Data {
	std::array<Clock::Divider::type, Model::NumChans> cdiv;
	bool Validate() const {
		auto ret = true;
		for (auto &c : cdiv) {
			ret &= c.Validate();
		}
		return ret;
	}
};

} // namespace Catalyst2::Legacy::V1_0::Sequencer::Phaser
