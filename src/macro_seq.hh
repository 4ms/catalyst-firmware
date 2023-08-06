#pragma once
#include "conf/model.hh"
#include "flags.hh"
#include "params.hh"
#include "util/countzip.hh"
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

		if (params.mode == Params::Mode::Macro && params.macromode == Params::MacroMode::Classic)
			for (auto [chan, out] : countzip(buf)) {
				// use params to figure out how to fill the output buffer
				auto a = params.part.get_chan(0, chan);
				auto b = params.part.get_chan(1, chan);
				auto phase = params.morph_step + params.cv_offset;
				phase = std::clamp(phase, -1.f, 1.f);
				out = linear_interpolate(a, b, phase);
			}

		return buf;
	}

private:
	Scene::ChannelValue_t linear_interpolate(auto &from, auto &to, float phase)
	{
		auto temp = from * (1.f - phase);
		temp += to * phase;
		return static_cast<Scene::ChannelValue_t>(temp);
	}
};

} // namespace Catalyst2
