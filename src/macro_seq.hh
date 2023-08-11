#pragma once
#include "conf/model.hh"
#include "flags.hh"
#include "params.hh"
#include "util/countzip.hh"
#include "util/math.hh"
#include <algorithm>

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

		if (params.mode == Params::Mode::Macro && params.macromode == Params::MacroMode::Classic) {
			auto phase = params.morph_step;
			// phase += params.cv_offset;
			phase = std::clamp(phase, 0.f, .999f);
			auto left = params.pathway.scene_left(phase);
			auto right = params.pathway.scene_right(phase);
			phase = params.pathway.adjust_and_scale(phase);

			for (auto [chan, out] : countzip(buf)) {
				auto a = params.banks.get_chan(left, chan);
				auto b = params.banks.get_chan(right, chan);
				out = MathTools::interpolate(a, b, phase);
			}
		}

		return buf;
	}

private:
};

} // namespace Catalyst2
