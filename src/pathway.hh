#pragma once
#include "fixed_list.hh"
#include "scene.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Pathway {
private:
	static constexpr auto near_threshold = 1.f / Model::fader_width_mm * 2.5f;
	float scene_width;

public:
	static constexpr size_t MaxPoints = 64; //???how many? is this model-specific?
	using SceneId = unsigned;				// could be a Scene ptr?
	FixedFwList<SceneId, MaxPoints> path;
	unsigned index_left;
	unsigned index_nearest;
	bool on_a_scene;

	Pathway()
	{
		path.insert(0, 0);
		path.insert(1);
		path.insert(2);
		path.insert(3);
		path.insert(4);
		path.insert(5);
		path.insert(6);
		path.insert(7);
		update_scene_width();
	}

	void update(float point)
	{
		on_a_scene = scene_is_near(point);
		index_left = phase_to_index(point);
		index_nearest = phase_to_index(point + (scene_width * .5f));
	}

	SceneId scene_left()
	{
		return path.read(index_left);
	}
	SceneId scene_right()
	{
		return path.read(index_left + 1);
	}
	SceneId scene_nearest()
	{
		return path.read(index_nearest);
	}

	bool replace_scene(SceneId scene)
	{
		return path.replace(index_nearest, scene);
	}

	// insert scene on path
	bool insert_scene(SceneId scene, bool after_last = false)
	{
		auto out = false;

		if (after_last)
			out = path.insert(scene);
		else
			out = path.insert(index_left, scene);
		update_scene_width();
		return out;
	}

	// remove scene
	bool remove_scene()
	{
		bool out = path.erase(index_nearest);
		update_scene_width();
		return out;
	}

	void clear_scenes()
	{
		// erase all scenes in between first and last one.
		while (path.size() > 2 && path.erase(1))
			;
		update_scene_width();
	}

	unsigned size()
	{
		return path.size();
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
	bool scene_is_near(float point)
	{
		while (point > scene_width)
			point -= scene_width;
		if (point != std::clamp(point, near_threshold, scene_width - near_threshold))
			return true;

		return false;
	}
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
