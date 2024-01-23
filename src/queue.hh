#pragma once

#include "conf/model.hh"
#include "sequencer_settings.hh"
#include <array>
#include <type_traits>

namespace Catalyst2::Sequencer::Queue
{
namespace Details
{

class Channel {
	struct State {
		uint8_t queued_page;
		bool is_queued = false;
	};
	std::array<State, Model::NumChans> state;

public:
	void Queue(uint8_t chan, uint8_t page) {
		state[chan].queued_page = page;
		state[chan].is_queued = true;
	}
	uint8_t Step(uint8_t chan) {
		state[chan].is_queued = false;
		return state[chan].queued_page;
	}
	bool IsQueued(uint8_t chan) {
		return state[chan].is_queued;
	}
};

struct Data {
	std::array<uint8_t, Model::MaxQueuedStartOffsetPages + 1> queue;
	uint8_t size;
	bool Validate() const {
		auto ret = true;
		for (auto &q : queue) {
			ret &= q < Model::SeqPages;
		}
		ret &= size <= Model::MaxQueuedStartOffsetPages;
		return ret;
	}
};

class Global {
	std::array<bool, Model::NumChans> is_queued;
	std::array<uint8_t, Model::NumChans> pos;
	bool is_looping = false;
	Data &data;

public:
	Global(Data &data)
		: data{data} {
	}

	void Queue(uint8_t page, bool do_loop) {
		if (!do_loop) {
			data.queue[Model::MaxQueuedStartOffsetPages] = page;
			data.size = 0;
			is_looping = false;
		} else {
			if (!is_looping) {
				data.queue[0] = data.queue[Model::MaxQueuedStartOffsetPages];
				data.queue[1] = page;
				data.size = 2;
				for (auto &p : pos) {
					p = 0;
				}
			} else {
				if (data.size >= Model::MaxQueuedStartOffsetPages) {
					return;
				}
				data.queue[data.size] = page;
				data.size += 1;
			}
			is_looping = true;
		}
		for (auto &iq : is_queued) {
			iq = true;
		}
	}
	void Step(uint8_t chan) {
		if (is_looping) {
			pos[chan] += 1;
			if (pos[chan] >= data.size) {
				pos[chan] = 0;
			}
		} else {
			is_queued[chan] = false;
		}
	}
	uint8_t Read() const {
		return data.queue[Model::MaxQueuedStartOffsetPages];
	}
	uint8_t Read(uint8_t chan) const {
		return data.queue[pos[chan]];
	}
	bool IsQueued(uint8_t chan) const {
		return is_queued[chan];
	}
	bool HasFinished() const {
		if (is_looping) {
			return false;
		}
		auto ret = true;
		for (auto &iq : is_queued) {
			ret &= iq == false;
		}
		return ret;
	}
	void Cancel() {
		for (auto &iq : is_queued) {
			iq = false;
		}
	}
	void Reset() {
		for (auto &p : pos) {
			p = 0;
		}
	}
	bool IsLooping() const {
		return is_looping;
	}
};

} // namespace Details

using Data = Details::Data;

struct Interface {
	Details::Channel channel;
	Details::Global global;
	Interface(Data &data)
		: global{data} {
	}
};
} // namespace Catalyst2::Sequencer::Queue
