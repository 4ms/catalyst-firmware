#pragma once
#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

class Sequencer {
	using SequenceId = uint8_t;

	struct Sequence {
		uint8_t length;
		uint8_t step;
	};
	std::array<Sequence, Model::NumChans> sequence;
	SequenceId cur_chan = 0;

public:
	Sequencer()
	{
		for (auto &s : sequence) {
			s.step = 0;
			s.length = Model::NumChans;
		}
	}

	void sel_chan(SequenceId chan)
	{
		if (chan >= Model::NumChans)
			return;

		cur_chan = chan;
	}

	SequenceId get_sel_chan()
	{
		return cur_chan;
	}

	void step()
	{
		for (auto &s : sequence) {
			s.step += 1;
			if (s.step == s.length)
				s.step = 0;
		}
	}

	void reset()
	{
		for (auto &s : sequence) {
			s.step = 0;
		}
	}

	void set_length(SequenceId seq, uint8_t length)
	{
		if (length > Model::NumChans || length == 0)
			return;

		sequence[seq].length = length;
	}

	uint8_t get_length(SequenceId seq)
	{
		return sequence[seq].length;
	}

	uint8_t get_step(SequenceId seq)
	{
		return sequence[seq].step;
	}
};

} // namespace Catalyst2
