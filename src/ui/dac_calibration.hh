#pragma once

#include "channel.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "helper_functions.hh"
#include "util/countzip.hh"
#include <array>

namespace Catalyst2::Calibration::Dac
{

// 100 millivolts up or down
inline constexpr auto max_adjustment_volts = .1f;

inline constexpr int16_t max_offset = Channel::Output::from_volts(-5 + max_adjustment_volts);
inline constexpr int16_t min_offset = -max_offset;

inline constexpr int16_t max_slope = Channel::Output::max / Model::output_octave_range * max_adjustment_volts;
inline constexpr int16_t min_slope = -max_slope;

struct Data {
	struct Channel {
		int16_t offset = 0;
		int16_t slope = 0;
	};
	std::array<Channel, Model::NumChans> channel{};

	bool Validate() const {
		unsigned all_bits_set = true;

		for (auto &c : channel) {
			if (c.offset < min_offset || c.offset > max_offset)
				return false;
			if (c.slope < min_slope || c.slope > max_slope)
				return false;

			if (c.offset != -1 || c.slope != -1)
				all_bits_set = false;
		}

		// If all bits are set, then the data is likely to be erased flash (all 0xFF bytes)
		// So we should reject this data
		if (all_bits_set)
			return false;

		return true;
	}
};

inline int32_t ApplySlope(int32_t in, float slope) {
	slope /= static_cast<float>(max_slope);
	slope *= max_adjustment_volts;
	in -= Channel::Output::from_volts(0.f);
	in *= 1.f + slope;
	return in + Channel::Output::from_volts(0.f);
}

inline Model::Output::type ApplyOffset(int32_t in, int16_t offset) {
	return std::clamp<int32_t>(in + offset, Channel::Output::min, Channel::Output::max);
}

inline Model::Output::type Process(const Data::Channel &d, Model::Output::type input) {
	input = ApplySlope(input, d.slope);
	return ApplyOffset(input, d.offset);
}

inline bool Calibrate(Data &d, Controls &c) {
	auto idx = 0;
	const auto prev_ = d;
	c.SetButtonLed(idx, true);
	while (true) {
		c.Update();

		using namespace Channel::Output;
		static constexpr std::array test_voltage = {
			from_volts(-4.75f),
			from_volts(-3.f),
			from_volts(0.f),
			from_volts(2.f),
			from_volts(4.f),
			from_volts(5.f),
			from_volts(8.f),
			from_volts(9.75f),
		};

		Model::Output::Buffer out;

		Ui::ForEachSceneButtonJustPressed(c, [&c, &idx](uint8_t button) {
			c.SetButtonLed(idx, false);
			idx = button;
			c.SetButtonLed(idx, true);
		});

		for (auto [idx, o, c] : enumerate(out, d.channel)) {
			o = Process(c, test_voltage[idx]);
		}
		auto color_func = [&c](uint8_t idx, float phase) {
			auto col = Palette::blue;
			if (phase < 0.f) {
				col = Palette::red;
				phase *= -1.f;
			}
			c.SetEncoderLed(idx, Palette::off.blend(col, phase));
		};
		if (c.button.shift.is_pressed()) {
			for (auto i = 0u; i < Model::NumChans; i++) {
				d.channel[i].offset =
					std::clamp<int16_t>(d.channel[i].offset + c.encoders[i].read(), min_offset, max_offset);
				const auto phase = d.channel[i].offset / static_cast<float>(max_offset);
				color_func(i, phase);
			}
		} else if (c.button.fine.is_pressed()) {
			for (auto i = 0u; i < Model::NumChans; i++) {
				d.channel[i].slope =
					std::clamp<int16_t>(d.channel[i].slope + c.encoders[i].read(), min_slope, max_slope);
				const auto phase = d.channel[i].slope / static_cast<float>(max_slope);
				color_func(i, phase);
			}
		} else {
			for (auto &i : c.encoders) {
				(void)i.read();
			}
			Ui::SetEncoderLedsCount(c, 8, 0, Palette::off);
		}

		if (c.button.play.just_went_high()) {
			if (c.button.bank.is_pressed()) {
				d = Data{};
			} else {
				d = prev_;
				return false;
			}
		}
		if (c.button.bank.is_pressed() && c.button.morph.is_pressed()) {
			return true;
		}

		c.Write(out);
		c.Delay(1);
	}
}
} // namespace Catalyst2::Calibration::Dac
