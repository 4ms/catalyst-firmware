#pragma once
#include "conf/model.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Macro::SliderSlew
{

enum class Curve : uint8_t { Linear, Expo };
static constexpr float MaxSlew = Model::sample_rate_hz * 120.f;
static constexpr float EncoderStepSize = 1.f / 200.f;

inline float CalcCoef(float slew) {
	if (slew <= EncoderStepSize)
		return 1.f;
	if (slew >= (1.f - EncoderStepSize))
		return 1.f / MaxSlew;

	auto rate = slew * slew * slew * slew * MaxSlew;
	if (rate > 1.f / MaxSlew)
		return 1.f / rate;

	return 1.f / MaxSlew;
}

struct Data {
	float slew{0.f};
	float coef{CalcCoef(slew)};
	Curve curve{Curve::Linear};

	bool Validate() {
		return (std::abs(CalcCoef(slew) - coef) < 0.0001f) && (slew >= 0.f && slew <= 1.f) &&
			   (static_cast<uint8_t>(curve) == static_cast<uint8_t>(Curve::Linear) ||
				static_cast<uint8_t>(curve) == static_cast<uint8_t>(Curve::Expo));
	}
};

class Interface {
	Data &data;

	float current{0.f};

public:
	Interface(Data &data)
		: data{data} {
	}

	void Inc(int32_t inc) {
		auto slew = data.slew + inc * EncoderStepSize;
		data.slew = std::clamp(slew, 0.f, 1.f);
		data.coef = CalcCoef(data.slew);
	}

	float Value() const {
		return data.slew;
	}

	float Update(float new_val) {
		return data.curve == Curve::Linear ? UpdateLinear(new_val) : UpdateExpo(new_val);
	}

	float UpdateExpo(float new_val) {
		current += (new_val - current) * data.coef;
		return current;
	}

	float UpdateLinear(float new_val) {
		// Rough adjustment to make linear vs expo curves perceived as more similar in rate of change
		auto lin_coef = data.coef / 3.f;

		if (new_val > current) {
			current = std::clamp(current + lin_coef, current, new_val);
		} else if (new_val < current) {
			current = std::clamp(current - lin_coef, new_val, current);
		}

		return current;
	}

	void SetCurve(Curve new_curve) {
		data.curve = new_curve;
	}

	Curve GetCurve() const {
		return data.curve;
	}
};
} // namespace Catalyst2::Macro::SliderSlew
