#pragma once
#include "fixedvector.hh"
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

	FixedVector<SceneId, MaxPoints> path;
	float scene_width;
	unsigned index_left;
	unsigned index_nearest;
	bool on_a_scene_;
	unsigned prev_index = 0;

public:
	Pathway()
	{
		path.insert(0, 0);
		path.insert(1, 7);
		update_scene_width();
	}
	void refresh()
	{
		update_scene_width();
	}
	void update(float point)
	{
		on_a_scene_ = scene_is_near(point);
		index_left = phase_to_index(point);
		auto n = phase_to_index(point + (scene_width * .5f));
		index_nearest = n >= size() ? 0 : n;
	}

	SceneId scene_left()
	{
		return path[index_left];
	}
	SceneId scene_right()
	{
		auto idx = index_left + 1;
		idx = idx >= size() ? 0 : idx;
		return path[idx];
	}
	SceneId scene_nearest()
	{
		return path[index_nearest];
	}
	bool on_a_scene()
	{
		return on_a_scene_;
	}
	void replace_scene(SceneId scene)
	{
		path[index_nearest] = scene;
		prev_index = index_nearest;
	}

	void insert_scene(SceneId scene, bool after_last = false)
	{
		auto index = index_left;

		if (after_last)
			index = prev_index++;
		else
			prev_index = index + 1;

		path.insert(index, scene);
		update_scene_width();
	}

	void remove_scene()
	{
		if (size() <= 2)
			return;

		path.erase(index_nearest);
		update_scene_width();
	}

	void clear_scenes()
	{
		// erase all scenes in between first and last one.
		while (size() > 2)
			path.erase(1);

		update_scene_width();
	}

	std::size_t size() const
	{
		return path.size();
	}

	float get_scene_width() const
	{
		return scene_width;
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
		auto out = static_cast<unsigned>(phase * (size() - 1));
		return out % size();
	}
	void update_scene_width()
	{
		scene_width = 1.f / (size() - 1);
	}
};

} // namespace Catalyst2
