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

		// TODO: test
		params.pathway.update(params.pos);

		if (params.mode == Params::Mode::Macro) {
			auto left = params.pathway.scene_left();
			auto right = params.pathway.scene_right();

			auto phase = params.pos;
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
		auto morph = params.morph_step;

		// this avoids dividing by 0
		if (morph >= 1.f) {
			return in < .5f ? 0.f : 1.f;
		}

		morph = ((1.f / (1.f - morph)) * (in - .5f)) + .5f;
		return std::clamp(morph, 0.f, 1.f);
	}
};

} // namespace Catalyst2
