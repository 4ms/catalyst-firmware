#pragma once
#include "conf/board_conf.hh"
#include "flags.hh"
#include "params.hh"

namespace Catalyst2
{

class MacroSeq {
	Params &params;
	Flags &flags;

public:
	MacroSeq(Params &params, Flags &flags)
		: params{params}
		, flags{flags}
	{}

	Board::OutputBuffer update()
	{
		Board::OutputBuffer buf;

		for (auto &b : buf) {
			b = 0;
		}

		return buf;
	}
};

} // namespace Catalyst2
