#pragma once
#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Sequencer {
	using SequenceId = uint8_t;
	struct Sequence {
		uint32_t length : 6;
		uint32_t counter : 6;
		uint32_t step : 6;
		uint32_t start_offset : 6;
	};
	std::array<Sequence, Model::NumChans> sequence;
	SequenceId cur_chan = 0;

	Sequencer()
	{
		for (auto &s : sequence) {
			s.counter = 0;
			s.step = 0;
			s.start_offset = 0;
			s.length = Model::NumChans - 1;
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

	bool get_clock() const
	{
		return clock_;
	}

	void step()
	{
		clock_ = !clock_;

		for (auto &s : sequence) {
			s.counter += 1;
			if (s.counter >= s.length + 1)
				s.counter = 0;
			s.step = s.counter + s.start_offset;
			s.step &= 7;
		}
	}

	void reset()
	{
		for (auto &s : sequence) {
			s.counter = 0;
		}
	}

	void adj_length(SequenceId chan, int dir)
	{
		int temp = sequence[chan].length;

		temp = adjust(temp, dir);

		sequence[chan].length = temp;
	}

	void adj_start_offset(SequenceId chan, int dir)
	{
		int temp = sequence[chan].start_offset;

		temp = adjust(temp, dir);

		sequence[chan].start_offset = temp;
	}

	void reset_length(SequenceId seq)
	{
		sequence[seq].length = 7;
	}

	void reset_start_offset(SequenceId seq)
	{
		sequence[seq].start_offset = 0;
	}

	uint8_t get_length(SequenceId seq)
	{
		return sequence[seq].length + 1;
	}

	uint8_t get_start_offset(SequenceId seq)
	{
		return sequence[seq].start_offset;
	}

	uint8_t get_step(SequenceId seq)
	{
		return sequence[seq].step;
	}

private:
	bool clock_ = false;

	int adjust(int val, int dir)
	{
		val += dir;

		if (val < 0)
			val = 0;
		else if (val >= 8)
			val = 7;

		return val;
	}
};

} // namespace Catalyst2
