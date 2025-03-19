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

inline constexpr auto near_threshold = 1.f / Model::fader_width_mm * 2.5f;
inline constexpr auto gate_threshold = 1.f / 256;

inline constexpr float CalcSceneWidth(uint32_t size) {
	return 1.f / (size - 1u);
}

inline bool SceneIsNear(float point, float scene_width, float threshold = near_threshold) {
	while (point > scene_width) {
		point -= scene_width;
	}

	// this prevents undefined clamp behaviour
	// after twelve or so scenes are in the the path a scene is always near with a 2.5mm threshold.
	auto high = scene_width - threshold;
	if (high < threshold) {
		high = threshold;
	}

	return point != std::clamp(point, threshold, high);
}

struct PathwayData {
	static constexpr auto MaxPoints = 64u;
	static constexpr auto MinPoints = 2u;

	SceneId Read(uint32_t idx) const {
		return vec[idx];
	}
	void Set(uint32_t idx, SceneId d) {
		vec[idx] = d;
	}
	void Insert(uint32_t idx, SceneId d) {
		vec.insert(idx, d);
	}
	void Erase(uint32_t idx) {
		vec.erase(idx);
	}
	uint32_t size() const {
		return vec.size();
	}
	bool Validate() const {
		if (vec.size() > MaxPoints || vec.size() < MinPoints) {
			return false;
		}
		for (auto &i : vec) {
			if (i >= Model::Macro::NumScenes) {
				return false;
			}
		}
		return true;
	}

private:
	FixedVector<SceneId, MaxPoints> vec{SceneId{0}, SceneId{7}};
};

struct Data {
	PathwayData &operator[](uint32_t idx) {
		return pathways[idx];
	}
	void Clear(uint32_t idx) {
		pathways[idx] = PathwayData{};
	}
	bool Validate() const {
		auto ret = true;
		for (auto &pathway : pathways) {
			ret &= pathway.Validate();
		}
		return ret;
	}

private:
	std::array<PathwayData, Model::Macro::Bank::NumTotal> pathways{};
};

class Interface {
	PathwayData *pathway;
	SceneId scene_left = 0;
	SceneId scene_right = 0;
	SceneId scene_nearest = 0;
	SceneId prev_index = 0;
	bool on_a_scene = false;
	bool fire_gate = false;
	float phase = 0.f;
	float scene_width = 0.f;

public:
	void Load(PathwayData &d) {
		pathway = &d;
		scene_width = CalcSceneWidth(size());
	}

	void Update(float point) {
		const auto s = size();
		on_a_scene = SceneIsNear(point, scene_width);
		fire_gate = SceneIsNear(point, scene_width, gate_threshold);
		const auto path_phase = point * (s - 1);
		scene_left = static_cast<uint32_t>(path_phase) % s;
		scene_nearest = static_cast<uint32_t>(path_phase + (scene_width * .5f)) % s;
		scene_right = scene_left + 1 == s ? 0 : scene_left + 1;
		phase = path_phase - static_cast<uint32_t>(path_phase);
	}
	// classic fucntions
	void ReplaceSceneA(SceneId scene) {
		pathway->Set(0, scene);
	}
	void ReplaceSceneB(SceneId scene) {
		pathway->Set(1, scene);
	}
	SceneId GetSceneA() const {
		return pathway->Read(0);
	}
	SceneId GetSceneB() const {
		return pathway->Read(1);
	}
	float GetPhase() const {
		return phase;
	}
	SceneId SceneRelative(int8_t pos = 0) const {
		return pathway->Read(Relative(pos));
	}
	bool OnAScene() const {
		return on_a_scene;
	}
	bool FireGate() const {
		return fire_gate;
	}
	std::optional<SceneId> CurrentScene() {
		if (on_a_scene) {
			return SceneRelative();
		} else {
			return std::nullopt;
		}
	}
	std::optional<SceneId> CurrentGateScene() {
		if (fire_gate) {
			return SceneRelative();
		} else {
			return std::nullopt;
		}
	}
	void ReplaceScene(SceneId scene) {
		pathway->Set(scene_nearest, scene);
		prev_index = scene_nearest;
	}
	void InsertScene(SceneId scene) {
		prev_index = scene_left + 1;
		pathway->Insert(prev_index, scene);
		scene_width = CalcSceneWidth(size());
	}
	void InsertSceneAfterLast(SceneId scene) {
		prev_index++;
		pathway->Insert(prev_index, scene);
		scene_width = CalcSceneWidth(size());
	}

	void RemoveSceneRelative(int8_t pos = 0) {
		if (size() <= PathwayData::MinPoints) {
			return;
		}

		pathway->Erase(Relative(pos));
		scene_width = CalcSceneWidth(size());
	}

	uint8_t size() {
		return pathway->size();
	}

private:
	SceneId Relative(int8_t pos) const {
		return pos == 0 ? scene_nearest : pos == -1 ? scene_left : scene_right;
	}
};
} // namespace Catalyst2::Macro::Pathway
