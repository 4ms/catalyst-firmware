#pragma once
#include "conf/model.hh"
#include "util/fixed_vector.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace Catalyst2::Macro::Pathway
{
using SceneId = uint8_t;
static constexpr auto MaxPoints = 64u;

struct Data {
	Data() {
		vec.insert(0, 0);
		vec.insert(1, 7);
	}
	bool Validate() {
		for (auto &s : vec) {
			if (s >= Model::NumScenes) {
				return false;
			}
		}
		return vec.size() <= MaxPoints && vec.size() >= 2u;
	}

	FixedVector<SceneId, MaxPoints> vec;
};

class Interface {
	static constexpr auto near_threshold = 1.f / Model::fader_width_mm * 2.5f;

	Data *p;
	float scene_width;
	SceneId scene_left;
	SceneId scene_right;
	SceneId scene_nearest;
	SceneId prev_index = 0;
	bool on_a_scene;
	float phase;
	std::optional<SceneId> last_scene_on;

public:
	void Load(Data &d) {
		p = &d;
		UpdateSceneWidth();
	}

	void Update(float point) {
		last_scene_on = CurrentScene();
		on_a_scene = SceneIsNear(point);
		scene_left = PhaseToIndex(point);
		scene_right = scene_left + 1 >= size() ? 0 : scene_left + 1;
		auto n = PhaseToIndex(point + (scene_width * .5f));
		scene_nearest = n >= size() ? 0 : n;
		phase = point / scene_width;
		phase -= static_cast<uint32_t>(phase);
	}

	float GetPhase() {
		return phase;
	}
	SceneId SceneRelative(int8_t pos = 0) {
		return p->vec[Relative(pos)];
	}
	bool OnAScene() {
		return on_a_scene;
	}
	std::optional<SceneId> LastSceneOn() {
		return last_scene_on;
	}
	std::optional<SceneId> CurrentScene() {
		if (on_a_scene) {
			return SceneRelative();
		} else {
			return std::nullopt;
		}
	}
	void ReplaceScene(SceneId scene) {
		p->vec[scene_nearest] = scene;
		prev_index = scene_nearest;
	}
	void InsertScene(SceneId scene) {
		prev_index = scene_left + 1;
		p->vec.insert(prev_index, scene);
		UpdateSceneWidth();
	}
	void InsertSceneAfterLast(SceneId scene) {
		prev_index++;
		p->vec.insert(prev_index, scene);
		UpdateSceneWidth();
	}

	void RemoveSceneRelative(int8_t pos = 0) {
		if (size() <= 2) {
			return;
		}

		p->vec.erase(Relative(pos));
		UpdateSceneWidth();
	}

	void ClearScenes() {
		// erase all scenes in between first and last one.
		while (size() > 2)
			p->vec.erase(1);

		UpdateSceneWidth();
	}

	float GetSceneWidth() const {
		return scene_width;
	}

	uint8_t size() {
		return p->vec.size();
	}

private:
	SceneId Relative(int8_t pos) {
		return pos == 0 ? scene_nearest : pos == -1 ? scene_left : scene_right;
	}
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
} // namespace Catalyst2::Macro::Pathway
