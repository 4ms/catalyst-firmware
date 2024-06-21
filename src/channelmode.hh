#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "quantizer.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Channel
{
class Mode {
	static constexpr uint8_t max = Quantizer::scale.size() + Model::num_custom_scales;
	static_assert(Palette::Scales::color.size() - 1 == max);
	static_assert(max < 0x80,
				  "Note: In order to have more than 127 scales, the saved data structure will have to change size, "
				  "data recovery will be necessary");
	static constexpr uint8_t min = 0;

	uint8_t val = min;

public:
	void Inc(int32_t dir) {
		auto temp = Val();
		temp = std::clamp<int32_t>(temp + dir, min, max);
		val = (val & 0x80) | temp;
	}
	bool IsGate() const {
		return Val() == max;
	}
	uint8_t GetScaleIdx() const {
		return Val();
	}
	Color GetColor() const {
		return Palette::Scales::color[Val()];
	}
	bool IsCustomScale() const {
		return Val() >= Quantizer::scale.size() && !IsGate();
	}
	bool Validate() const {
		return Val() <= max;
	}
	bool IsMuted() const {
		return val & 0x80;
	}
	void ToggleMute() {
		val ^= 0x80;
	}

private:
	uint8_t Val() const {
		return val & 0x7f;
	}

}; // namespace Catalyst2::Channel
} // namespace Catalyst2::Channel
