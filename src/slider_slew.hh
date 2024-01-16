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

	static constexpr float MaxSlewCoef = 1.f / 10000.f;

	static constexpr float EncoderStepSize = 0.01f;

	static float CalcCoef(float slew) {
		// 0..1 => 1 (no slew) .. MaxSlewCoef
		// coef = 1.f + (MaxSlewCoef - 1.f) * data.slew;
		if (slew == 0)
			return 1.f;
		slew = 1.f - slew;
		return 1.f / (slew * 100.f + MaxSlewCoef);
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
