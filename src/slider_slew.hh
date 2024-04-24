#pragma once
#include "conf/model.hh"
#include "legacy/v1_0/conf/model.hh"
#include "validate.hh"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace Catalyst2::Macro::SliderSlew
{

enum class Curve : uint8_t { Linear, Expo };
static constexpr float MaxTime = 120.f; // seconds
static constexpr float MinSlew = 0.04f; // TODO: when constexpr math in gcc:  = std::powf(MaxSlew, 0.25);
static constexpr float EncoderStepSizeFine = 1.f / 200.f;
static constexpr float EncoderStepSizeCourse = EncoderStepSizeFine * 10;

// Converts slew amount(0..1) to a coef (1..1/MaxSlew)
inline constexpr float CalcCoef(float slew, float SampleRate) {
	const auto MaxSlew = SampleRate * MaxTime;
	slew = slew * (1.f - MinSlew) + MinSlew;
	slew = slew * slew * slew * slew * MaxSlew;
	if (slew < 1.f)
		return 1.f;

	return 1.f / slew;
}

struct Data {
	float slew{0.f};
	float coef{CalcCoef(slew, Model::sample_rate_hz)};
	Curve curve{Curve::Linear};

	void PostLoad() {
		coef = CalcCoef(slew, Model::sample_rate_hz);
	}

	void PreSave() {
		coef = CalcCoef(slew, Legacy::V1_0::Model::sample_rate_hz);
	}

	bool Validate() const {
		auto ret = true;
		ret &= slew >= 0.f && slew <= 1.f;
		ret &= std::abs(CalcCoef(slew, Legacy::V1_0::Model::sample_rate_hz) - coef) < 0.0001f;
		ret &= std::to_underlying(curve) <= std::to_underlying(Curve::Expo);
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

	void Inc(int32_t inc, bool fine) {
		auto slew = data.slew + inc * (fine ? EncoderStepSizeFine : EncoderStepSizeCourse);
		data.slew = std::clamp(slew, 0.f, 1.f);
		data.coef = CalcCoef(data.slew, Model::sample_rate_hz);
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
