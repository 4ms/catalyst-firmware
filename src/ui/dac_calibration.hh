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
inline constexpr auto max_offset = Channel::Output::from_volts(-4.9);
inline constexpr auto min_offset = -max_offset;

inline constexpr auto slope_scale = 100000.f;
inline constexpr auto r_slope_scale = 1.f / slope_scale;
inline constexpr int16_t max_slope =
	((Channel::Output::max + max_offset) / static_cast<float>(Channel::Output::max) - 1.f) * slope_scale;
inline constexpr int16_t min_slope = -max_slope;

struct Data {
	struct Channel {
		int16_t offset = 0;
		int16_t slope = 0;
	};
	std::array<Channel, Model::NumChans> channel{};

	bool Validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.offset >= min_offset && c.offset <= max_offset;
			ret &= c.slope >= min_slope && c.slope <= max_slope;
		}
		return ret;
	}
};

inline void Process(Data &d, Model::Output::Buffer &input) {
	for (auto [i, in] : enumerate(input)) {
		int32_t temp = in * (1.f + (d.channel[i].slope * r_slope_scale));
		in = std::clamp<int32_t>(temp + d.channel[i].offset, Channel::Output::min, Channel::Output::max);
	}
}

inline bool Calibrate(Data &d, Controls &c) {
	auto idx = 0;
	const auto prev_ = d;
	c.SetButtonLed(idx, true);
	while (true) {
		c.Update();

		using namespace Channel::Output;
		static constexpr std::array test_voltage = {
			from_volts(-5.f),
			from_volts(-3.f),
			from_volts(0.f),
			from_volts(1.f),
			from_volts(3.f),
			from_volts(5.f),
			from_volts(8.f),
			from_volts(10.f),
		};

		Model::Output::Buffer out;

		Ui::ForEachSceneButtonJustPressed(c, [&c, &idx](uint8_t button) {
			c.SetButtonLed(idx, false);
			idx = button;
			c.SetButtonLed(idx, true);
		});

		for (auto &o : out) {
			o = test_voltage[idx];
		}
		auto color_func = [&c](uint8_t idx, float phase) {
			auto col = Palette::blue;
			if (phase < 0.f) {
				col = Palette::red;
				phase *= -1.f;
			}
			c.SetEncoderLed(idx, Palette::off.blend(col, phase));
		};
		if (c.button.shift.is_high()) {
			for (auto i = 0u; i < Model::NumChans; i++) {
				d.channel[i].offset =
					std::clamp<int16_t>(d.channel[i].offset + c.encoders[i].read(), min_offset, max_offset);
				const auto phase = d.channel[i].offset / static_cast<float>(max_offset);
				color_func(i, phase);
			}
		} else if (c.button.fine.is_high()) {
			for (auto i = 0u; i < Model::NumChans; i++) {
				d.channel[i].slope =
					std::clamp<int16_t>(d.channel[i].slope + c.encoders[i].read(), min_slope, max_slope);
				const auto phase = d.channel[i].slope / static_cast<float>(max_slope);
				color_func(i, phase);
			}
		} else {
			Ui::SetEncoderLedsCount(c, 8, 0, Palette::off);
		}

		if (c.button.add.is_high()) {
			d = prev_;
			return false;
		}
		if (c.button.play.is_high()) {
			d = Data{};
			return true;
		}
		if (c.button.bank.is_high() && c.button.morph.is_high()) {
			return true;
		}

		Process(d, out);

		c.Write(out);
		c.Delay(1);
	}
}
} // namespace Catalyst2::Calibration::Dac
