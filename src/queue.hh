#pragma once

#include "conf/model.hh"
#include "sequencer_settings.hh"
#include <array>

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
class Global {
	using bit_type = std::conditional_t<Model::NumChans <= 8, uint8_t, uint32_t>;
	static constexpr bit_type queue_mask = static_cast<bit_type>(-1);
	bit_type is_queued = 0;
	uint8_t queued_page;

public:
	void Queue(uint8_t page) {
		is_queued = queue_mask;
		queued_page = page;
	}
	void Step(uint8_t chan) {
		is_queued &= ~(1u << chan);
	}
	uint8_t Read() const {
		return queued_page;
	}
	bool IsQueued(uint8_t chan) const {
		return is_queued & (1u << chan);
	}
	bool HasFinished() const {
		return is_queued == 0;
	}
};
} // namespace Details

struct Interface {
	Details::Channel channel;
	Details::Global global;
};
} // namespace Catalyst2::Sequencer::Queue
