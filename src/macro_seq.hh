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

		if (params.mode == Params::Mode::Macro && params.macromode == Params::MacroMode::Classic) {
			auto phase = params.morph_step;
			phase += params.cv_offset;
			phase = std::clamp(phase, 0.f, 1.f);
			auto left = params.pathway.nearest_scene(Pathway::Vicinity::NearestLeft, phase);
			phase = params.pathway.adjust_and_scale(phase);

			for (auto [chan, out] : countzip(buf)) {
				auto a = params.part.get_chan(left.scene, chan);
				auto b = a;
				if (left.next != nullptr)
					b = params.part.get_chan(left.next->scene, chan);

				// use params to figure out how to fill the output buffer
				out = linear_interpolate(a, b, phase);
			}
		}

		return buf;
	}

private:
	Scene::ChannelValue_t linear_interpolate(Scene::ChannelValue_t &from, Scene::ChannelValue_t &to, float phase)
	{
		auto temp = from * (1.f - phase);
		temp += to * phase;
		return static_cast<Scene::ChannelValue_t>(temp);
	}
};

} // namespace Catalyst2
