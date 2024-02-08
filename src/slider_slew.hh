#pragma once
#include "conf/model.hh"
#include "validate.hh"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace Catalyst2::Macro::SliderSlew
{

enum class Curve : bool { Linear, Expo };
static constexpr float MaxTime = Model::sample_rate_hz * 120.f;
static constexpr float MinSlew = 0.04f; // TODO: when constexpr math in gcc:  = std::powf(MaxTime, 0.25);
static constexpr float EncoderStepSize = 1.f / 200.f;

// Converts slew amount(0..1) to a coef (1..1/MaxSlew)
inline float CalcCoef(float slew) {
	slew = slew * (1.f - MinSlew) + MinSlew;
	slew = slew * slew * slew * slew * MaxTime;
	if (slew < 1.f)
		return 1.f;

	return 1.f / slew;
}

struct Data {
	float slew{0.f};
	float coef{CalcCoef(slew)};
	Curve curve{Curve::Linear};

	bool Validate() {
		auto ret = true;
		ret &= std::abs(CalcCoef(slew) - coef) < 0.0001f;
		ret &= slew >= 0.f && slew <= 1.f;
		ret &= validateBool(std::to_underlying(curve));
		return ret;
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
