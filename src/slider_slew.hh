#pragma once
#include "conf/model.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Macro::SliderSlew
{

struct Data {
	float slew{0.f};
};

class Interface {
	Data &data;

	enum class Curve { Linear, Expo } mode{Curve::Linear};
	float coef;
	float current{0.f};

	static constexpr float MaxSlew = Model::sample_rate_hz * 120.f;
	static constexpr float EncoderStepSize = 1.f / 200.f;

	static float CalcCoef(float slew) {
		if (slew <= EncoderStepSize)
			return 1.f;
		if (slew >= (1.f - EncoderStepSize))
			return 1.f / MaxSlew;

		auto rate = slew * slew * slew * slew * MaxSlew;
		if (rate > 1.f / MaxSlew)
			return 1.f / rate;

		return 1.f / MaxSlew;
	}

public:
	Interface(Data &data)
		: data{data}
		, coef{CalcCoef(data.slew)} {
	}

	void Inc(int32_t inc) {
		auto slew = data.slew + inc * EncoderStepSize;
		data.slew = std::clamp(slew, 0.f, 1.f);
		coef = CalcCoef(data.slew);
	}

	float Value() {
		return data.slew;
	}

	float Update(float new_val) {
		return mode == Curve::Linear ? UpdateLinear(new_val) : UpdateExpo(new_val);
	}

	float UpdateExpo(float new_val) {
		current += (new_val - current) * coef;
		return current;
	}

	float UpdateLinear(float new_val) {
		if (new_val > current) {
			current = std::clamp(current + coef, current, new_val);
		} else if (new_val < current) {
			current = std::clamp(current - coef, new_val, current);
		}

		return current;
	}

	void LinearMode() {
		mode = Curve::Linear;
	}

	void ExpoMode() {
		mode = Curve::Expo;
	}
};
} // namespace Catalyst2::Macro::SliderSlew
