#pragma once
#include "fixedvector.hh"
#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2::Pathway
{
using SceneId = uint8_t;
static constexpr auto MaxPoints = 64u;
using Data = FixedVector<SceneId, MaxPoints>;

class Interface {
	static constexpr auto near_threshold = 1.f / Model::fader_width_mm * 2.5f;

	Data *p;
	float scene_width;
	SceneId scene_left;
	SceneId scene_nearest;
	SceneId prev_index = 0;
	bool on_a_scene;

public:
	void Load(Data &d) {
		p = &d;
		if (size() < 2) {
			p->insert(0, 0);
			p->insert(1, 7);
		}
		UpdateSceneWidth();
	}

	void Update(float point) {
		on_a_scene = SceneIsNear(point);
		scene_left = PhaseToIndex(point);
		auto n = PhaseToIndex(point + (scene_width * .5f));
		scene_nearest = n >= size() ? 0 : n;
	}

	SceneId SceneLeft() {
		return (*p)[scene_left];
	}
	SceneId SceneRight() {
		auto idx = scene_left + 1;
		idx = idx >= size() ? 0 : idx;
		return (*p)[idx];
	}
	SceneId SceneNearest() {
		return (*p)[scene_nearest];
	}
	bool OnAScene() {
		return on_a_scene;
	}
	void ReplaceScene(SceneId scene) {
		(*p)[scene_nearest] = scene;
		prev_index = scene_nearest;
	}

	void InsertScene(SceneId scene, bool after_last) {
		auto index = scene_left;

		if (after_last)
			index = prev_index++;
		else
			prev_index = index + 1;

		p->insert(index, scene);
		UpdateSceneWidth();
	}

	void RemoveSceneNearest() {
		if (size() <= 2)
			return;

		p->erase(scene_nearest);
		UpdateSceneWidth();
	}

	void RemoveSceneLeft() {
		if (size() <= 2)
			return;

		p->erase(scene_left);
		UpdateSceneWidth();
	}

	void RemoveSceneRight() {
		if (size() <= 2)
			return;

		auto idxr = scene_left + 1;
		idxr = size() ? 0 : idxr;

		p->erase(idxr);
		UpdateSceneWidth();
	}

	void ClearScenes() {
		// erase all scenes in between first and last one.
		while (size() > 2)
			p->erase(1);

		UpdateSceneWidth();
	}

	float GetSceneWidth() const {
		return scene_width;
	}

	uint8_t size() {
		return p->size();
	}

private:
	bool SceneIsNear(float point) {
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
	unsigned PhaseToIndex(float phase) {
		auto out = static_cast<unsigned>(phase * (size() - 1));
		return out % size();
	}
	void UpdateSceneWidth() {
		scene_width = 1.f / (size() - 1);
	}
};
} // namespace Catalyst2::Pathway
