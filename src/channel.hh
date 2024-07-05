#pragma once

#include "conf/model.hh"
#include "range.hh"
#include "util/math.hh"
#include <algorithm>
#include <bit>
#include <cstdint>
#include <numeric>

namespace Catalyst2::Channel
{
namespace Cv
{
using type = uint16_t;
inline constexpr auto notes_in_octave = 12u;
inline constexpr type inc_step_fine = 1u;
inline constexpr type inc_step = 25u;
inline constexpr type min = 0u;
inline constexpr type max = static_cast<uint32_t>(Model::output_octave_range * notes_in_octave * inc_step);
inline constexpr type bits = std::bit_width(max);
inline constexpr type zero = MathTools::map_value(0.f, Model::min_output_voltage, Model::max_output_voltage, min, max);

inline constexpr type octave = max / Model::output_octave_range;
inline constexpr type note = octave / notes_in_octave;

inline constexpr type RangeToMin(Range range) {
	return MathTools::map_value(range.NegAmount(), .5f, 0.f, min, zero);
}
inline constexpr type RangeToMax(Range range) {
	return MathTools::map_value(range.PosAmount(), 0.f, 1.f, zero, max);
}

inline type Inc(type val, int32_t inc, bool fine, Range range) {
	inc *= fine ? inc_step_fine : inc_step;
	return std::clamp<int32_t>(val + inc, RangeToMin(range), RangeToMax(range));
}
inline type Read(type val, float random) {
	const auto t = val + (random * max);
	return std::clamp<int32_t>(t, min, max);
}
inline bool Validate(type val) {
	return val <= max;
}

}; // namespace Cv
namespace Gate
{
using internal_type = uint8_t;
using type = float;

inline constexpr internal_type min = 0u;
inline constexpr internal_type max = 15u;
inline constexpr internal_type bits = std::bit_width(max);

inline internal_type Inc(internal_type val, int32_t inc) {
	return std::clamp<int32_t>(val + inc, min, max);
}
inline type Read(internal_type val, float random) {
	const auto t = val + (random * max);
	return std::clamp(t / static_cast<type>(max), type(0), type(1));
}
inline bool Validate(internal_type val) {
	return val <= max;
}
}; // namespace Gate
namespace Output
{
inline constexpr Model::Output::type max = UINT16_MAX;
inline constexpr Model::Output::type min = 0;
inline constexpr Model::Output::type range = max - min;

inline constexpr Model::Output::type from_volts(const float volts) {
	auto v = std::clamp(volts, Model::min_output_voltage, Model::max_output_voltage);
	return MathTools::map_value(v, Model::min_output_voltage, Model::max_output_voltage, min, max);
}

inline constexpr float to_semitone(Model::Output::type raw) {
	constexpr auto div = (UINT16_MAX + 1.f) / Model::output_octave_range / 12;
	return raw / div;
}

inline constexpr float to_octave(Model::Output::type raw) {
	constexpr auto div = (UINT16_MAX + 1.f) / (Model::output_octave_range);
	return raw / div;
}

inline constexpr Model::Output::type gate_armed = from_volts(0.f);
inline constexpr Model::Output::type gate_off = gate_armed - 1;
inline constexpr Model::Output::type gate_high = from_volts(10.f);

inline constexpr Model::Output::type ScaleCv(Cv::type val, Cv::Range range) {
	const auto t = val / static_cast<float>(Cv::max);
	return std::clamp<int32_t>(t * Output::max, from_volts(range.Min()), from_volts(range.Max()));
}
inline constexpr Model::Output::type ScaleGate(Model::Output::type val, Cv::Range range) {
	return std::clamp<int32_t>(val, min, from_volts(range.Max()));
}

} // namespace Output
} // namespace Catalyst2::Channel
