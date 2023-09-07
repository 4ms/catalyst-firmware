#pragma once
#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Sequencer {
	using SequenceId = uint8_t;
	struct Sequence {
		uint8_t length;
		uint8_t step;
		uint8_t start_offset;
	};
	std::array<Sequence, Model::NumChans> sequence;
	SequenceId cur_chan = 0;

	Sequencer()
	{
		for (auto &s : sequence) {
			s.step = 0;
			s.start_offset = 0;
			s.length = Model::NumChans;
		}
	}

	bool is_chan_selected()
	{
		return cur_chan < Model::NumChans;
	}

	void sel_chan(SequenceId chan)
	{
		if (chan >= Model::NumChans || chan == cur_chan) {
			cur_chan = Model::NumChans;
			return;
		}

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
			if (s.step >= s.length)
				s.step = 0;
		}
	}

	void reset()
	{
		for (auto &s : sequence) {
			s.step = 0;
		}
	}

	void adj_length(SequenceId chan, int dir)
	{
		if (dir < 0)
			dir = -1;
		else if (dir > 0)
			dir = 1;

		sequence[chan].length += dir;

		if (sequence[chan].length == 0)
			sequence[chan].length = 1;
		else if (sequence[chan].length > 8)
			sequence[chan].length = 8;
	}

	void adj_start_offset(SequenceId chan, int dir)
	{
		int temp = sequence[chan].start_offset;

		temp += dir;

		if (temp < 0)
			temp = 0;
		else if (temp >= 8)
			temp = 7;

		sequence[chan].start_offset = temp;
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

	uint8_t get_start_offset(SequenceId seq)
	{
		return sequence[seq].start_offset;
	}

	uint8_t get_step(SequenceId seq)
	{
		return (sequence[seq].step + sequence[seq].start_offset) % 8;
	}
};

} // namespace Catalyst2
