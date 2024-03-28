#pragma once

#include "conf/model.hh"
#include "sequencer_settings.hh"
#include <array>

namespace Catalyst2::Legacy::V1_0::Sequencer::SongMode
{

struct Data {
	std::array<uint8_t, Model::Sequencer::MaxQueuedStartOffsetPages> queue;
	uint8_t size;
	bool Validate() const {
		auto ret = true;
		for (auto &q : queue) {
			ret &= q < Model::Sequencer::NumPages;
		}
		ret &= size <= queue.size();
		return ret;
	}
};

} // namespace Catalyst2::Legacy::V1_0::Sequencer::SongMode
