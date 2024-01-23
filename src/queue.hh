#pragma once

#include "conf/model.hh"
#include "sequencer_settings.hh"
#include <array>

namespace Catalyst2::Sequencer::Queue
{

class Interface {
	using type = Settings::StartOffset::type;
	struct State {
		type prev_offset;
		bool is_queued = false;
	};
	std::array<State, Model::NumChans> state;
	Settings::Data &settings;

public:
	Interface(Settings::Data &settings)
		: settings{settings} {
	}
	void Queue(uint8_t chan, uint8_t page) {
		if (!state[chan].is_queued) {
			state[chan].prev_offset = settings.GetStartOffsetOrGlobal(chan);
			state[chan].is_queued = true;
		}
		settings.SetStartOffset(chan, page * Model::SeqStepsPerPage);
	}
	void Queue(uint8_t page) {
		const auto so = settings.GetStartOffset();
		for (auto i = 0u; i < Model::NumChans; ++i) {
			if (state[i].is_queued || settings.GetStartOffset(i).has_value()) {
				// if value has override ignore...
				continue;
			}
			state[i].prev_offset = so;
			state[i].is_queued = true;
		}
		settings.SetStartOffset(page * Model::SeqStepsPerPage);
	}
	type Read(uint8_t chan) {
		if (state[chan].is_queued) {
			return state[chan].prev_offset;
		} else {
			return settings.GetStartOffsetOrGlobal(chan);
		}
	}
	void Step(uint8_t chan) {
		state[chan].is_queued = false;
	}
	void Stop() {
		for (auto &s : state) {
			s.is_queued = false;
		}
	}
};

} // namespace Catalyst2::Sequencer::Queue
