#pragma once

#include "channel.hh"
#include "conf/model.hh"
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

		Ui::ForEachSceneButtonJustPressed(c, [&idx](uint8_t button) { idx = button; });

		for (auto &o : out) {
			o = test_voltage[idx];
		}
		if (c.button.shift.is_high()) {
			Ui::ForEachEncoderInc(c, [&d](uint8_t encoder, int32_t inc) {
				d.channel[encoder].offset =
					std::clamp<int16_t>(d.channel[encoder].offset + inc, min_offset, max_offset);
			});
		} else if (c.button.fine.is_high()) {
			Ui::ForEachEncoderInc(c, [&d](uint8_t encoder, int32_t inc) {
				d.channel[encoder].slope = std::clamp<int16_t>(d.channel[encoder].slope + inc, min_slope, max_slope);
			});
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
