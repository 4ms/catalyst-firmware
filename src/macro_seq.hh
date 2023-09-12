#pragma once
#include "conf/model.hh"
//#include "conf/quantizer_scales.hh"
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

		params.pathway.update(params.pos);

		if (params.mode == Params::Mode::Macro)
			macro(buf);
		else
			seq(buf);

		for (auto &out : buf) {
			out = params.quantizer.process(out);
		}

		return buf;
	}

private:
	void macro(auto &in)
	{
		if (params.override_output.has_value()) {
			for (auto [chan, out] : countzip(in))
				out = params.banks.get_chan(params.override_output.value(), chan);
			return;
		}

		const auto left = params.pathway.scene_left();
		const auto right = params.pathway.scene_right();

		auto phase = params.pos;
		phase = params.pathway.adjust_and_scale(phase);
		phase = MathTools::slope_adj(phase, params.morph_step, 0.f, 1.f);

		for (auto [chan, out] : countzip(in)) {
			const auto a = params.banks.get_chan(left, chan);
			const auto b = params.banks.get_chan(right, chan);
			out = MathTools::interpolate(a, b, phase);
		}
	}
	void seq(Model::OutputBuffer &in)
	{
		for (auto [chan, out] : countzip(in)) {
			const auto step = params.seq.get_step(chan);
			out = params.banks.get_chan(step, chan);
		}
		in = rotate_(in, params.pos);
	}

	Model::OutputBuffer rotate_(const Model::OutputBuffer &in, float point)
	{
		static constexpr float width = 1.f / (Model::NumChans - 1);

		auto distance = 0u;

		while (point >= width) {
			point -= width;
			++distance;
		}

		point *= Model::NumChans;

		Model::OutputBuffer out;

		for (auto i = 0u; i < Model::NumChans; ++i) {
			auto idx = (Model::NumChans - i + distance);
			auto idx_next = idx + 1;
			if constexpr (MathTools::is_power_of_2(Model::NumChans)) {
				idx &= Model::NumChans - 1;
				idx_next &= Model::NumChans - 1;
			} else {
				idx %= Model::NumChans;
				idx_next %= Model::NumChans;
			}
			out[i] = MathTools::interpolate(in[idx], in[idx_next], point);
		}

		return out;
	}
};

} // namespace Catalyst2
