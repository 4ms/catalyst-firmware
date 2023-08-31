#pragma once
#include "fixed_list.hh"
#include "scene.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Pathway {
	using SceneId = unsigned;				// could be a Scene ptr?
	static constexpr size_t MaxPoints = 64; //???how many? is this model-specific?

private:
	static constexpr auto near_threshold = 1.f / Model::fader_width_mm * 2.5f;

	FixedFwList<SceneId, MaxPoints> path;
	float scene_width;
	unsigned index_left;
	unsigned index_nearest;
	bool on_a_scene_;

public:
	Pathway()
	{
		path.insert(0, 0);
		path.insert(7);
		update_scene_width();
	}

	void update(float point)
	{
		on_a_scene_ = scene_is_near(point);
		index_left = phase_to_index(point);
		index_nearest = phase_to_index(point + (scene_width * .5f));
	}

	SceneId scene_left()
	{
		return path.read(index_left);
	}
	SceneId scene_right()
	{
		if (index_left + 1 == path.size())
			return path.read(0);
		else
			return path.read(index_left + 1);
	}
	SceneId scene_nearest()
	{
		if (index_nearest == path.size())
			return path.read(0);
		else
			return path.read(index_nearest);
	}
	bool on_a_scene()
	{
		return on_a_scene_;
	}
	bool replace_scene(SceneId scene)
	{
		return path.replace(index_nearest, scene);
	}

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

	bool remove_scene()
	{
		if (path.size() == 2)
			return false;

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
		while (point >= scene_width)
			point -= scene_width;
		point *= path.size() - 1;
		return point;
	}

private:
	bool scene_is_near(float point)
	{
		while (point >= scene_width)
			point -= scene_width;

		// this prevents undefined clamp behaviour
		// after twelve or so scenes are in the the path a scene is always near with a 2.5mm threshold.
		auto high = scene_width - near_threshold;
		if (high < near_threshold)
			high = near_threshold;

		if (point != std::clamp(point, near_threshold, high))
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
