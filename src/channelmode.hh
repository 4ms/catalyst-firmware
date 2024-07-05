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

	static constexpr uint8_t gate_idx = Quantizer::scale.size();
	uint8_t val = min;

public:
	void Inc(int32_t dir) {
		auto temp = Val();
		if (dir > 0 && temp == gate_idx) {
			return;
		}
		if (dir > 0 && temp >= max) {
			temp = gate_idx;
		} else if (temp == gate_idx) {
			temp = max;
		} else {
			temp = std::clamp<int32_t>(temp + dir, min, max);
			if (temp == gate_idx) {
				temp = std::clamp<int32_t>(temp + dir, min, max);
			}
		}
		val = (val & 0x80) | temp;
	}
	bool IsGate() const {
		return Val() == gate_idx;
	}
	uint8_t GetScaleIdx() const {
		return IsCustomScale() ? Val() - 1 : Val();
	}
	Color GetColor() const {
		return IsGate() ? Palette::Scales::color.back() : Palette::Scales::color[GetScaleIdx()];
	}
	bool IsCustomScale() const {
		return Val() > gate_idx;
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
