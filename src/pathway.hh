#pragma once
#include "scene.hh"
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Pathway {
	static constexpr unsigned MaxPoints = 64; //???how many? is this model-specific?
	using SceneId = unsigned;				  // could be a Scene ptr?

	struct PathPoint {
		SceneId scene;
		float loc;
	};

	// TODO: is array the best way? double-linked-list? do they need to be in order?
	// Do we re-order on insert, or calculate nearest on calls to nearest_scene_*()?
	std::array<PathPoint, MaxPoints> path;

	// TODO; these are just ideas for an interface, take it or leave it!
	void insert_scene(SceneId scene, float location)
	{}

	PathPoint nearest_scene_left(float point)
	{
		// nearest scene to the left
		return {};
	}

	PathPoint nearest_scene_right(float point)
	{
		// nearest scene to the right
		return {};
	}
};

} // namespace Catalyst2
