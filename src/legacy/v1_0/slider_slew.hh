#pragma once
#include "conf/model.hh"
#include "validate.hh"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace Catalyst2::Legacy::V1_0::Macro::SliderSlew
{

enum class Curve : uint8_t { Linear, Expo };
static constexpr float MaxTime = Model::sample_rate_hz * 120.f;
static constexpr float MinSlew = 0.04f; // TODO: when constexpr math in gcc:  = std::powf(MaxTime, 0.25);
static constexpr float EncoderStepSizeFine = 1.f / 200.f;
static constexpr float EncoderStepSizeCourse = EncoderStepSizeFine * 10;

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

	bool Validate() const {
		auto ret = true;
		ret &= std::abs(CalcCoef(slew) - coef) < 0.0001f;
		ret &= slew >= 0.f && slew <= 1.f;
		ret &= validateBool(std::to_underlying(curve));
		return ret;
	}
};

} // namespace Catalyst2::Legacy::V1_0::Macro::SliderSlew
