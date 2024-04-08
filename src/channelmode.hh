#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "quantizer.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Channel
{
class Mode {
	static_assert(Palette::Scales::color.size() - 1 == Quantizer::scale.size());
	static constexpr uint8_t max = Quantizer::scale.size();
	static constexpr uint8_t min = 0u;

	int8_t val = min;

public:
	void Inc(int32_t dir) {
		auto temp = val + dir;
		val = std::clamp<int32_t>(temp, min, max);
	}
	bool IsGate() const {
		return val == max;
	}
	bool IsQuantized() const {
		return !IsGate();
	}
	const Quantizer::Scale &GetScale() const {
		return Quantizer::scale[val];
	}
	Color GetColor() {
		return Palette::Scales::color[val];
	}
	bool Validate() const {
		return val <= max;
	}

}; // namespace Catalyst2::Channel
} // namespace Catalyst2::Channel
