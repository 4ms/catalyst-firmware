#pragma once

#include "conf/model.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Channel
{

constexpr Model::Output::type max = UINT16_MAX;
constexpr Model::Output::type min = 0;
constexpr Model::Output::type range = max - min;

constexpr Model::Output::type from_volts(const float volts) {
	auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
	return MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, min, max);
}

class Range {
	struct Option {
		float min, max;
	};
	static constexpr std::array options = {
		Option{Model::min_output_voltage, Model::max_output_voltage},
		Option{-5.f, 5.f},
		Option{0.f, 10.f},
		Option{0.f, 5.f},
		Option{0.f, 3.f},
		Option{0.f, 1.f},
	};
	static_assert(
		[] {
			for (auto i : options) {
				if (i.min < Model::min_output_voltage || i.max > Model::max_output_voltage)
					return false;
			}
			return true;
		}(),
		"Range option cannot be outside the voltage range of the hardware");

	static constexpr uint8_t min = 0u;
	static constexpr uint8_t max = options.size() - 1;
	static constexpr auto absmaxv = [] {
		const auto min = Model::min_output_voltage < 0 ? -Model::min_output_voltage : Model::min_output_voltage;
		const auto max = Model::max_output_voltage < 0 ? -Model::max_output_voltage : Model::max_output_voltage;
		return max >= min ? max : min;
	}();
	uint8_t val = 0;

public:
	void Inc(int32_t inc) {
		val = std::clamp<int32_t>(val + inc, min, max);
	}
	float NegAmount() const {
		return std::abs(options[val].min / absmaxv);
	}
	float PosAmount() const {
		return options[val].max / absmaxv;
	}
	Model::Output::type Min() const {
		return from_volts(options[val].min);
	}
	Model::Output::type Max() const {
		return from_volts(options[val].max);
	}
	Model::Output::type Clamp(Model::Output::type in) {
		return std::clamp(in, Min(), Max());
	}
	float GetOctaveRange() const {
		return options[val].max - options[val].min;
	}
	bool Validate() const {
		return val <= max;
	}
};

} // namespace Catalyst2::Channel