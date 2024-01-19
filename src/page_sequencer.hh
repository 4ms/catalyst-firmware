#pragma once

#include "conf/model.hh"
#include "drivers/rotary_tables.hh"
#include "sequencer_settings.hh"
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace Catalyst2::Sequencer::Queue
{
namespace Channel
{
class Interface {
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
} // namespace Channel
namespace Global
{

struct Data {
	std::array<uint8_t, Model::max_queued_start_offset_changes> queue{};
	uint8_t length = 0;

	bool Validate() {
		auto ret = true;
		for (auto &c : queue) {
			ret &= c < Model::SeqPages;
		}
		ret &= length <= Model::max_queued_start_offset_changes;
		return ret;
	}
};

class Interface {
	struct State {
		uint8_t pos;
		bool did_reset = false;
	};
	std::array<State, Model::NumChans> state;
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	//	void Step(uint8_t chan) {
	//		auto &s = state[chan];
	//		s.pos += 1;
	//		if (s.pos >= data.length) {
	//			s.pos = 0;
	//			if (data.status == Status::ONE_SHOT) {
	//				s.did_reset = true;
	//			}
	//		}
	//	}
	//	void Queue(uint8_t page) {
	//		data.queue[0] = page;
	//		data.length = 1;
	//		data.status = Status::ONE_SHOT;
	//		for (auto &s : state) {
	//			s.did_reset = false;
	//		}
	//	}
	//	void Append(uint8_t page) {
	//		if (data.status == Status::ONE_SHOT) {
	//			data.status = Status::LOOP;
	//		}
	//		data.queue[data.length] = page;
	//		data.length += 1;
	//	}
	//	bool IsFull() {
	//		return data.length == Model::max_queued_start_offset_changes;
	//	}
	//	void Clear() {
	//		data.length = 0;
	//	}
	//	bool IsEnabled(uint8_t chan) {
	//		return state[chan].did_reset == false;
	//	}
	//	uint8_t Read(uint8_t chan) {
	//		return data.queue[state[chan].pos];
	//	}
};
} // namespace Global
} // namespace Catalyst2::Sequencer::Queue
