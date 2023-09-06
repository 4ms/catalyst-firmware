#pragma once
#include "conf/model.hh"
#include "flags.hh"
#include "params.hh"
#include "util/countzip.hh"
#include "util/math.hh"
#include <algorithm>

namespace MathTools
{
// if slope == 0 actual slope == 1
// if slope is >= (max - min) actual slope == inf
// if slope is negative than actual slope is less than 1
constexpr float slope_adj(float in, float slope, float min, float max)
{
	const auto range = max - min;
	const auto b = range / 2.f;

	if (slope >= range) {
		return in < b ? min : max;
	}

	const auto m = range / (range - slope);
	const auto x = in - b;
	const auto y = (m * x) + b;
	return constrain(y, min, max);
}
} // namespace MathTools

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
			const auto left = params.pathway.scene_left();
			const auto right = params.pathway.scene_right();

			auto phase = params.pos;
			phase = params.pathway.adjust_and_scale(phase);
			phase = MathTools::slope_adj(phase, params.morph_step, 0.f, 1.f);

			for (auto [chan, out] : countzip(buf)) {
				const auto a = params.banks.get_chan(left, chan);
				const auto b = params.banks.get_chan(right, chan);
				out = MathTools::interpolate(a, b, phase);
			}

			return buf;
		}

		// sequencer mode
		for (auto [chan, out] : countzip(buf)) {
			auto step = params.seq.get_step(chan);
			out = params.banks.get_chan(step, chan);
		}

		return buf;
	}
};

} // namespace Catalyst2
