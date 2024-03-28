#pragma once
#include "conf/model.hh"
#include "util/fixed_vector.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

namespace Catalyst2::Legacy::V1_0::Macro::Pathway
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

struct PathwayData {
	static inline constexpr auto MaxPoints = 64u;

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
} // namespace Catalyst2::Legacy::V1_0::Macro::Pathway
