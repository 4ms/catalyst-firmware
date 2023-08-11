#pragma once
#include "fixed_list.hh"
#include "scene.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Pathway {
	static constexpr size_t MaxPoints = 8; //???how many? is this model-specific?
	static constexpr unsigned near_threshold = UINT16_MAX / Model::fader_width_mm * 2.5;
	using SceneId = unsigned; // could be a Scene ptr?
	FixedFwList<SceneId, MaxPoints> path{0, 7};
	float scene_width;

	Pathway()
	{
		path.insert(0, 1);
		path.insert(2);
		path.insert(3);
		path.insert(4);
		path.insert(5);
		path.insert(6);
		update_scene_width();
	}

	// TODO; these are just ideas for an interface, take it or leave it!
	bool insert_scene(float point, SceneId scene)
	{
		auto out = path.insert(phase_to_index(point), scene);
		update_scene_width();
		return out;
	}

	bool insert_scene(SceneId scene)
	{
		auto out = path.insert(scene);
		update_scene_width();
		return out;
	}

	bool remove_scene(float point)
	{
		bool out = path.erase(phase_to_index(point));
		update_scene_width();
		return out;
	}
	bool scene_is_near(float point)
	{
		auto p = static_cast<unsigned>(UINT16_MAX * point);
		auto mod = static_cast<unsigned>(UINT16_MAX * scene_width);
		p %= mod;
		if (p != std::clamp(p, near_threshold, mod - near_threshold))
			return true;

		return false;
	}
	bool is_between_scenes(float point)
	{
		return !scene_is_near(point);
	}
	SceneId nearest_scene(float point)
	{
		point += (scene_width * .5f);
		return scene_left(point);
	}
	SceneId scene_left(float point)
	{
		return path.read(phase_to_index(point));
	}
	SceneId scene_right(float point)
	{
		return path.read(phase_to_index(point) + 1);
	}

	// needs a better name
	float adjust_and_scale(float point) const
	{
		while (point > scene_width)
			point -= scene_width;
		point *= path.size() - 1;
		return point;
	}

private:
	unsigned phase_to_index(float phase)
	{
		return phase * (path.size() - 1);
	}
	void update_scene_width()
	{
		scene_width = 1.f / (path.size() - 1);
	}
};

} // namespace Catalyst2
