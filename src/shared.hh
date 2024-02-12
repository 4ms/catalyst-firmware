#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "quantizer.hh"
#include "validate.hh"
#include <optional>
#include <utility>

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
		uint32_t remaining{};
		uint32_t blink_duration{};
		uint32_t set_time{};
		uint32_t delay{};
	};
	std::array<State, Model::NumChans> state{};

public:
	void Set(uint8_t led, uint32_t num_blinks, uint32_t duration_ms, uint32_t time_now, uint32_t delay_ms = 0) {
		auto &s = state[led];
		s.delay = Clock::MsToTicks(delay_ms);
		s.remaining = num_blinks * 2;
		s.blink_duration = Clock::MsToTicks(duration_ms) / num_blinks;
		s.set_time = time_now;
	}
	void Set(uint32_t num_blinks, uint32_t duration_ms, uint32_t time_now, uint32_t delay_ms = 0) {
		for (auto i = 0u; i < state.size(); i++) {
			Set(i, num_blinks, duration_ms, time_now, delay_ms);
		}
	}
	void Cancel(uint8_t led) {
		state[led].remaining = 0;
	}
	void Cancel() {
		for (auto i = 0u; i < state.size(); i++) {
			Cancel(i);
		}
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
	Model::Mode saved_mode = Model::default_mode;
	bool Validate() const {
		auto ret = true;
		ret &= validateBool(std::to_underlying(saved_mode));
		return ret;
	}
};

struct Dummy {
	bool Validate() const {
		return true;
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
	std::optional<uint8_t> youngest_scene_button;
	Model::Mode mode = Model::default_mode;
};

} // namespace Catalyst2::Shared
