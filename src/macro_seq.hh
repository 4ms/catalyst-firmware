#pragma once
#include "conf/model.hh"
#include "flags.hh"
#include "params.hh"

namespace Catalyst2
{

class MacroSeq {
	Params &params;

public:
	MacroSeq(Params &params)
		: params{params}
	{}

	auto update()
	{
		Model::OutputBuffer buf;

		for (auto &b : buf) {
			// use params to figure out how to fill the output buffer
			if (params.mode == Params::Mode::Sequencer)
				b = 0;
		}

		return buf;
	}
};

} // namespace Catalyst2
