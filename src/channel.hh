#pragma once

#include "conf/model.hh"
#include "random.hh"
#include <algorithm>
#include <array>
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

constexpr auto gatehigh = from_volts(5.f);
constexpr auto gatearmed = from_volts(0.f) + 1u;
constexpr auto gateoff = gatearmed - 1u;

constexpr auto octave = range / Model::output_octave_range;
constexpr auto note = octave / 12;

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
	bool Validate() {
		return val <= max;
	}
};

class Value {
	static constexpr auto inc_step_fine = 1;
	static constexpr auto inc_step = inc_step_fine * 25;
	static constexpr auto min = 0, max = static_cast<int>(inc_step * 12 * Model::output_octave_range);
	static constexpr auto zero =
		MathTools::map_value(0.f, Model::min_output_voltage, Model::max_output_voltage, min, max);
	int16_t val;

public:
	class Proxy {
		const int32_t val;

	public:
		Proxy(int32_t offset)
			: val{offset} {
		}
		Model::Output::type AsCV() const {
			return val / static_cast<float>(max) * Channel::max;
		}
		bool AsGate() const {
			return val & 0x01;
		}
	};
	constexpr Value(float volts = 0.f) {
		auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
		val = MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, min, max);
	}
	void Inc(int32_t inc, bool fine, bool is_gate, Range range, Random::type offset) {
		if (is_gate) {
			if (inc > 0 && !(val & 0x01)) {
				val += 1;
			} else if (inc < 0 && (val & 0x01)) {
				val -= 1;
			}
		} else {
			inc *= fine ? inc_step_fine : inc_step;

			const auto o = CalculateRandom(range, offset);
			const auto min_ = MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
			const auto max_ = MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
			val = std::clamp<int32_t>(val + inc, min_ - o, max_ - o);
		}
	}
	Proxy Read(Range range, Random::type offset) const {
		const auto t = val + CalculateRandom(range, offset);
		const auto min_ = MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
		const auto max_ = MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
		return Proxy{std::clamp<int32_t>(t, min_, max_)};
	}

private:
	int32_t CalculateRandom(Range range, Random::type offset) const {
		offset *= offset < 0.f ? range.NegAmount() : range.PosAmount();
		return offset * max;
	}
};
} // namespace Catalyst2::Channel