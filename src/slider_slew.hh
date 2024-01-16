#pragma once
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Macro::SliderSlew
{

struct Data {
	float slew{0.f};
};

class Interface {
	Data &data;
	float coef;
	float current{0.f};

	static constexpr float MaxSlew = 100000.f;
	static constexpr float EncoderStepSize = 1.f / 200.f;

	static float CalcCoef(float slew) {
		if (slew <= EncoderStepSize)
			return 1.f;
		if (slew >= (1.f - EncoderStepSize))
			return 1.f / MaxSlew;

		return 1.f / (slew * slew * MaxSlew);
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
		current += (new_val - current) * coef;
		return current;
	}
};
} // namespace Catalyst2::Macro::SliderSlew
