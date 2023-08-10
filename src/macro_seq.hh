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
			phase += params.cv_offset;
			phase = std::clamp(phase, 0.f, 1.f);
			auto left = params.pathway.nearest_scene(Pathway::Vicinity::NearestLeft, phase);
			phase = params.pathway.adjust_and_scale(phase);

			for (auto [chan, out] : countzip(buf)) {
				auto a = params.banks.get_chan(left.scene, chan);
				auto b = a;
				if (left.next != nullptr)
					b = params.banks.get_chan(left.next->scene, chan);

				out = linear_interpolate(a, b, phase);
			}
		}

		return buf;
	}

private:
	// DG: Use values, not references for ChannelValue::type
	// Also, this is a general math function, no need to re-write
	// MathTools::interpolate(float, float, float) would be the same
	ChannelValue::type linear_interpolate(ChannelValue::type from, ChannelValue::type to, float phase)
	{
		auto temp = from * (1.f - phase);
		temp += to * phase;
		return static_cast<ChannelValue::type>(temp);
	}
};

} // namespace Catalyst2
