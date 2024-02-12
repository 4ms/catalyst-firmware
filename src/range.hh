#pragma once

#include "conf/model.hh"
#include <algorithm>
#include <cstdint>

namespace Catalyst2::Channel::Cv
{
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
	constexpr float NegAmount() const {
		return std::abs(options[val].min / absmaxv);
	}
	constexpr float PosAmount() const {
		return options[val].max / absmaxv;
	}
	float Min() const {
		return options[val].min;
	}
	float Max() const {
		return options[val].max;
	}
	bool Validate() const {
		return val <= max;
	}
};
} // namespace Catalyst2::Channel::Cv
