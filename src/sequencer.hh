#pragma once
#include "conf/model.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

class Sequencer {
	unsigned length_;
	unsigned cur_step_{0};

public:
	Sequencer(unsigned max_steps)
		: length_{max_steps}
	{}

	void step()
	{}

	void reset()
	{}

	void length(unsigned len)
	{
		length_ = len;
	}
	unsigned length()
	{
		return length_;
	}
	unsigned cur_step()
	{
		return cur_step_;
	}
};

} // namespace Catalyst2
