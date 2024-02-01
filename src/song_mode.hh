#pragma once

#include "conf/model.hh"
#include "sequencer_settings.hh"
#include <array>

namespace Catalyst2::Sequencer::SongMode
{

struct Data {
	std::array<uint8_t, Model::MaxQueuedStartOffsetPages> queue;
	uint8_t size;
	bool Validate() const {
		auto ret = true;
		for (auto &q : queue) {
			ret &= q < Model::SeqPages;
		}
		ret &= size <= queue.size();
		return ret;
	}
};

class Interface {
	struct State {
		uint8_t pos;
		bool is_queued = false;
	};
	std::array<State, Model::NumChans> state;
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	void Queue(uint8_t page) {
		if (data.size >= data.queue.size()) {
			return;
		}
		data.queue[data.size] = page;
		data.size += 1;
	}
	void Cancel() {
		if (data.size) {
			for (auto &s : state) {
				s.is_queued = true;
			}
		}
		data.size = 0;
	}
	uint8_t Size() const {
		return data.size;
	}
	Settings::StartOffset::type Read(uint8_t chan) const {
		return data.queue[state[chan].pos] * Model::SeqStepsPerPage;
	}
	bool IsActive() const {
		return data.size != 0;
	}
	void Step(uint8_t chan) {
		state[chan].pos += 1;
		if (state[chan].pos >= data.size) {
			state[chan].pos = 0;
		}
		state[chan].is_queued = false;
	}
	bool IsQueued(uint8_t chan) {
		return state[chan].is_queued;
	}
	bool IsQueued() {
		for (auto s : state) {
			if (s.is_queued) {
				return true;
			}
		}
		return false;
	}
};

} // namespace Catalyst2::Sequencer::SongMode
