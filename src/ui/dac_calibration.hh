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

struct Data {
	struct Channel {
		int16_t offset = 0.f;
		float slope = 0.f;
	};
	std::array<Channel, Model::NumChans> channel{};

	bool Validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.offset >= min_offset && c.offset <= max_offset;
			// TODO: slope
		}
		return ret;
	}
};

// TODO: slope
inline void Process(Data &d, Model::Output::Buffer &input) {
	for (auto [i, in] : enumerate(input)) {
		in = std::clamp<int32_t>(in + d.channel[i].offset, Channel::Output::min, Channel::Output::max);
	}
}

inline bool Calibrate(Data &d, Controls &c) {
	auto idx = 0;
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
		}

		if (c.button.play.is_high()) {
			return true;
		}
		if (c.button.add.is_high()) {
			return false;
		}

		Process(d, out);

		c.Write(out);
		c.Delay(1);
	}
}
} // namespace Catalyst2::Calibration::Dac
