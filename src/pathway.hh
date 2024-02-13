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

inline constexpr float CalcSceneWidth(uint32_t size) {
	return 1.f / (size - 1u);
}

inline uint32_t PhaseToIndex(float phase, uint32_t size) {
	const auto out = static_cast<uint32_t>(phase * (size - 1));
	return out % size;
}
inline bool SceneIsNear(float point, float scene_width) {
	static constexpr auto near_threshold = 1.f / Model::fader_width_mm * 2.5f;

	while (point >= scene_width) {
		point -= scene_width;
	}

	// this prevents undefined clamp behaviour
	// after twelve or so scenes are in the the path a scene is always near with a 2.5mm threshold.
	auto high = scene_width - near_threshold;
	if (high < near_threshold) {
		high = near_threshold;
	}

	return point != std::clamp(point, near_threshold, high);
}

namespace Abstract
{
struct Data {
	virtual SceneId Read(uint32_t idx) const = 0;
	virtual void Erase(uint32_t idx){};
	virtual void Set(uint32_t idx, SceneId d) = 0;
	virtual void Insert(uint32_t idx, SceneId d){};
	virtual uint32_t size() const = 0;
};
} // namespace Abstract

namespace Normal
{
inline constexpr auto MaxPoints = 64u;

struct Data : Abstract::Data {
	SceneId Read(uint32_t idx) const override {
		return vec[idx];
	}
	void Set(uint32_t idx, SceneId d) override {
		vec[idx] = d;
	}
	void Insert(uint32_t idx, SceneId d) override {
		vec.insert(idx, d);
	}
	void Erase(uint32_t idx) override {
		vec.erase(idx);
	}
	uint32_t size() const override {
		return vec.size();
	}
	bool Validate() const {
		if (vec.size() > MaxPoints || vec.size() < 2u) {
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
} // namespace Normal

namespace Classic
{
struct Data : Abstract::Data {
	SceneId Read(uint32_t idx) const override {
		return idx ? b : a;
	}
	void Set(uint32_t idx, SceneId d) override {
		idx ? b = d : a = d;
	}
	uint32_t size() const override {
		return 2;
	}
	bool Validate() const {
		return a < Model::Macro::NumScenes && b < Model::Macro::NumScenes;
	}

private:
	SceneId a = 0, b = 7;
};
} // namespace Classic

struct Data {
	Abstract::Data &operator[](uint32_t idx) {
		if (idx >= Model::Macro::Bank::NumNormal) {
			return classic;
		} else {
			return normal[idx];
		}
	}
	void Clear(uint32_t idx) {
		if (idx >= Model::Macro::Bank::NumNormal) {
			classic = Classic::Data{};
		} else {
			normal[idx] = Normal::Data{};
		}
	}
	bool Validate() const {
		auto ret = true;
		for (auto &i : normal) {
			ret &= i.Validate();
		}
		ret &= classic.Validate();
		return ret;
	}

private:
	std::array<Normal::Data, Model::Macro::Bank::NumNormal> normal{};
	Classic::Data classic{};
};

class Interface {
	Abstract::Data *data;
	SceneId scene_left = 0;
	SceneId scene_right = 0;
	SceneId scene_nearest = 0;
	SceneId prev_index = 0;
	bool on_a_scene = false;
	float phase = 0.f;
	std::optional<SceneId> last_scene_on;

public:
	void Load(Abstract::Data &d) {
		data = &d;
	}

	void Update(float point) {
		const auto s = size();
		const auto scene_width = CalcSceneWidth(s);
		last_scene_on = CurrentScene();
		on_a_scene = SceneIsNear(point, scene_width);
		scene_left = PhaseToIndex(point, s);
		scene_right = scene_left + 1 >= s ? 0 : scene_left + 1;
		auto n = PhaseToIndex(point + (scene_width * .5f), s);
		scene_nearest = n >= s ? 0 : n;
		phase = point / scene_width;
		phase -= static_cast<uint32_t>(phase);
	}
	// classic fucntions
	void ReplaceSceneA(SceneId scene) {
		data->Set(0, scene);
	}
	void ReplaceSceneB(SceneId scene) {
		data->Set(1, scene);
	}

	float GetPhase() const {
		return phase;
	}
	SceneId SceneRelative(int8_t pos = 0) const {
		return data->Read(Relative(pos));
	}
	bool OnAScene() const {
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
		data->Set(scene_nearest, scene);
		prev_index = scene_nearest;
	}
	void InsertScene(SceneId scene) {
		prev_index = scene_left + 1;
		data->Insert(prev_index, scene);
	}
	void InsertSceneAfterLast(SceneId scene) {
		prev_index++;
		data->Insert(prev_index, scene);
	}

	void RemoveSceneRelative(int8_t pos = 0) {
		if (size() <= 2) {
			return;
		}

		data->Erase(Relative(pos));
	}

	void ClearScenes() {
		// erase all scenes in between first and last one.
		while (size() > 2) {
			data->Erase(1);
		}
	}

	uint8_t size() {
		return data->size();
	}

private:
	SceneId Relative(int8_t pos) const {
		return pos == 0 ? scene_nearest : pos == -1 ? scene_left : scene_right;
	}
};
} // namespace Catalyst2::Macro::Pathway
