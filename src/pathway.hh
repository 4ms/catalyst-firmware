#pragma once
#include "scene.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

struct Pathway {
	static constexpr size_t MaxPoints = 8; //???how many? is this model-specific?
	using SceneId = unsigned;			   // could be a Scene ptr?

	uint8_t count{2};
	float scene_width{1.f};

	enum class Vicinity { NearestLeft, Absolute, NearestRight };

	struct PathPoint {
		SceneId scene = Model::NumChans;
		PathPoint *next = nullptr;
		PathPoint *prev = nullptr;
	};

	// TODO: is array the best way? double-linked-list? do they need to be in order?
	// Do we re-order on insert, or calculate nearest on calls to nearest_scene_*()?
	std::array<PathPoint, MaxPoints> path;
	PathPoint *head;

	Pathway()
	{
		path[0].next = &path[1];
		path[0].next->prev = &path[0];
		path[0].scene = 0;
		path[0].next->scene = 7;
		head = &path[0];
	}

	// TODO; these are just ideas for an interface, take it or leave it!
	bool insert_scene_before(SceneId scene, float point)
	{
		if (count == MaxPoints)
			return false;

		PathPoint *nearest = &nearest_scene(Vicinity::Absolute, point);
		PathPoint *new_ = nullptr;

		for (auto &temp : path) {
			if (temp.scene == Model::NumChans) {
				// unused node
				new_ = &temp;
				break;
			}
		}

		if (nearest == head)
			head = new_;

		new_->prev = nearest->prev;
		new_->next = nearest;
		new_->scene = scene;
		nearest->prev->next = new_;
		nearest->prev = new_;

		count += 1;

		set_scene_width();
		return true;
	}
	bool insert_scene_after(SceneId scene, float point)
	{
		if (count == MaxPoints)
			return false;

		PathPoint *nearest = &nearest_scene(Vicinity::Absolute, point);
		PathPoint *new_ = nullptr;

		for (auto &temp : path) {
			if (temp.scene == Model::NumChans) {
				// unused node
				new_ = &temp;
				break;
			}
		}

		new_->next = nearest->next;
		new_->prev = nearest;
		new_->scene = scene;
		nearest->next->prev = new_;
		nearest->next = new_;

		count += 1;

		set_scene_width();
		return true;
	}
	bool remove_scene(float point)
	{
		if (count == 2)
			return false;

		auto *remove = &nearest_scene(Vicinity::Absolute, point);

		if (remove == head) {
			// removing head, need to update.
			head = remove->next;
		}

		remove->prev->next = remove->next;
		remove->next->prev = remove->prev;

		remove->next = nullptr;
		remove->prev = nullptr;
		remove->scene = Model::NumChans;

		count -= 1;

		set_scene_width();

		return true;
	}

	PathPoint &nearest_scene(Vicinity v, float point)
	{
		auto index = point / scene_width;
		if (v == Vicinity::Absolute)
			index += .5f;
		else if (v == Vicinity::NearestRight)
			index += 1.f;

		auto temp = static_cast<size_t>(index);

		PathPoint *start = head;

		while (temp--)
			start = start->next;

		return *start;
	}

	float adjust_and_scale(float point)
	{
		while (point > scene_width)
			point -= scene_width;
		point *= count - 1;
		return point;
	}

private:
	void set_scene_width()
	{
		scene_width = 1.000000001f / (count - 1);
	}

	/*
	unsigned nearest_scene_right(float point)
	{
		// nearest scene to the right
		auto point_size = 1.f / count;
		auto pos = static_cast<unsigned>(point / point_size);
		return std::clamp(pos + 1, 0u, MaxPoints - 1);
	}
	*/
};

} // namespace Catalyst2
