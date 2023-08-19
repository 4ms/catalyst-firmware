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

		if (params.mode == Params::Mode::Macro) {
			auto left = params.pathway.scene_left();
			auto right = params.pathway.scene_right();

			auto phase = params.slider_pos;
			phase += params.cv_offset;
			phase = params.pathway.adjust_and_scale(phase);
			phase = do_morph(phase);

			for (auto [chan, out] : countzip(buf)) {
				auto a = params.banks.get_chan(left, chan);
				auto b = params.banks.get_chan(right, chan);
				out = MathTools::interpolate(a, b, phase);
			}
		}

		return buf;
	}

private:
	float do_morph(float in)
	{
		auto temp = ((1.f / (1.f - params.morph_step)) * (in - .5f)) + .5f;
		return std::clamp(temp, 0.f, 1.f);
	}
};

} // namespace Catalyst2
