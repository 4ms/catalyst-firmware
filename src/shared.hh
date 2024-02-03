#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "quantizer.hh"
#include <optional>

namespace Catalyst2::Shared
{
class DisplayHanger {
	static constexpr uint32_t duration = Clock::MsToTicks(4000);
	uint8_t onto;
	uint32_t start_time;

public:
	void Cancel() {
		onto = 0xff;
	}
	void Set(uint8_t encoder, uint32_t time_now) {
		start_time = time_now;
		onto = encoder;
	}
	std::optional<uint8_t> Check(uint32_t time_now) {
		if (time_now - start_time >= duration) {
			onto = 0xff;
		}
		if (onto == 0xff) {
			return std::nullopt;
		}
		return onto;
	}
};

class Blinker {
	struct State {
		uint32_t remaining;
		uint32_t blink_duration;
		uint32_t set_time;
		uint32_t delay;
	};
	std::array<State, Model::NumChans> state;

public:
	void Set(uint8_t led, uint32_t num_blinks, uint32_t duration_ms, uint32_t time_now, uint32_t delay_ms = 0) {
		auto &s = state[led];
		s.delay = Clock::MsToTicks(delay_ms);
		s.remaining = num_blinks * 2;
		s.blink_duration = Clock::MsToTicks(duration_ms) / num_blinks;
		s.set_time = time_now;
	}
	void Update(uint32_t time_now) {
		const auto t = time_now;
		for (auto &s : state) {
			if (s.delay) {
				s.delay--;
				continue;
			}
			if (!s.remaining) {
				continue;
			}
			if (t - s.set_time >= s.blink_duration) {
				s.set_time = t;
				s.remaining -= 1;
			}
		}
	}
	bool IsSet() const {
		for (auto &s : state) {
			if (s.remaining) {
				return true;
			}
		}
		return false;
	}
	bool IsHigh(uint8_t led) const {
		return state[led].remaining & 0x01;
	}
};

struct Data {
	Model::Mode mode = Model::default_mode;
	bool Validate() {
		auto ret = true;
		ret &= static_cast<uint8_t>(mode) == static_cast<uint8_t>(Model::Mode::Macro) ||
			   static_cast<uint8_t>(mode) == static_cast<uint8_t>(Model::Mode::Sequencer);
		return ret;
	}
};

class Interface {
	using QuantizerArray = std::array<Quantizer::Interface, Model::NumChans>;

public:
	Data &data;
	Interface(Data &data)
		: data{data} {
	}
	Clock::Internal internalclock;
	QuantizerArray quantizer;
	Clock::Divider clockdivider;
	DisplayHanger hang;
	Clock::Timer reset{Model::HoldTimes::reset};
	Clock::Timer modeswitcher{Model::HoldTimes::mode_switcher};
	Clock::Timer save{Model::HoldTimes::save};
	Blinker blinker;
	bool do_save = false;
	bool did_paste = false;
	bool did_copy = false;
	float pos;
};

} // namespace Catalyst2::Shared
