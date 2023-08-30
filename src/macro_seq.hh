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
// TODO: check if this is working properly
constexpr float slope_adj(float in, float slope, float min, float max)
{
	auto range = max - min;
	auto b = range / 2.f;

	if (slope >= range) {
		return in < b ? min : max;
	}

	auto m = range / (range - slope);
	auto x = in - b;
	auto y = m * x + b;
	return std::clamp(y, min, max);
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
			auto left = params.pathway.scene_left();
			auto right = params.pathway.scene_right();

			auto phase = params.pos;
			phase = params.pathway.adjust_and_scale(phase);
			phase = MathTools::slope_adj(phase, params.morph_step, 0.f, 1.f);

			for (auto [chan, out] : countzip(buf)) {
				auto a = params.banks.get_chan(left, chan);
				auto b = params.banks.get_chan(right, chan);
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

private:
	// this is really just a slope adjustment function
	// m = 1/1 -> ~1/.0000001
	// TODO: maybe this can become a math utils function
	// it would need more parameters
	// slope == 0, m = 1
	// slope == .999, m = 1000
	// not using for now, see function at top of this file
	// float do_morph(float in, float slope)
	// {
	// 	// this avoids dividing by 0
	// 	if (slope >= 1.f) {
	// 		return in < .5f ? 0.f : 1.f;
	// 	}
	// 	auto m = (1.f / (1.f - slope));
	// 	auto x = in - .5f;
	// 	static constexpr auto b = .5f;
	// 	auto y = m * x + b;
	// 	return std::clamp(y, 0.f, 1.f);
	// }
};

} // namespace Catalyst2
