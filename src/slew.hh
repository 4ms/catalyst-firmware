#pragma once

#include "conf/model.hh"
#include "legacy/v1_0/conf/model.hh"
#include "pathway.hh"
#include "util/countzip.hh"
#include "util/math.hh"
#include "validate.hh"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace Catalyst2::Macro::Slew
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

inline float UpdateExpo(Data &data, float current, float new_val) {
	return current + (new_val - current) * data.coef;
}

inline float ShapeExpo(float val) {
	return val;
}

inline float UpdateLinear(Data &data, float current, float new_val) {
	// Rough adjustment to make linear vs expo curves perceived as more similar in rate of change
	const auto lin_coef = data.coef / 3.f;

	if (new_val > current) {
		current = std::clamp(current + lin_coef, current, new_val);
	} else if (new_val < current) {
		current = std::clamp(current - lin_coef, new_val, current);
	}
	return current;
}

class Slider {
	Data &data;
	float pos;

public:
	Slider(Data &data)
		: data{data} {
	}

	float Update(float new_val) {
		pos = UpdateLinear(data, pos, new_val);
		if (data.curve == Curve::Expo) {
			return ShapeExpo(pos);
		}
		return pos;
	}
};

class Button {
	Data &data;

	float pos = 1.f;
	bool running = false;

public:
	Button(Data &data)
		: data{data} {
	}
	void Start() {
		pos = 0.f;
		running = true;
	}
	float Update() {
		if (!running) {
			return pos;
		}
		pos = UpdateLinear(data, pos, 1.f);
		if (data.curve == Curve::Expo) {
			return ShapeExpo(pos);
		}
		if (pos >= 1.f) {
			running = false;
		}
		return pos;
	}
	bool IsRunning() {
		return running;
	}
};

class Interface {
	Data &data;

public:
	Slider slider{data};
	Button button{data};

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
	void SetCurve(Curve new_curve) {
		data.curve = new_curve;
	}
	Curve GetCurve() const {
		return data.curve;
	}
	void Clear() {
		data = Data{};
	}
};
} // namespace Catalyst2::Macro::Slew
