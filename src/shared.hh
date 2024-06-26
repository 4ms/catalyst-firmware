#pragma once

#include "clock.hh"
#include "conf/build_options.hh"
#include "conf/model.hh"
#include "quantizer.hh"
#include "ui/dac_calibration.hh"
#include "validate.hh"
#include "youngest_scene_button.hh"
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
	void Set(uint8_t encoder) {
		start_time = Controls::TimeNow();
		onto = encoder;
	}
	std::optional<uint8_t> Check() {
		if (Controls::TimeNow() - start_time >= duration) {
			onto = 0xff;
		}
		if (onto == 0xff) {
			return std::nullopt;
		}
		return onto;
	}
	std::optional<uint8_t> Check() const {
		if (onto == 0xff)
			return std::nullopt;
		else
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
	void Set(uint8_t led, uint32_t num_blinks, uint32_t duration_ms, uint32_t delay_ms = 0) {
		auto &s = state[led];
		s.delay = Clock::MsToTicks(delay_ms);
		s.remaining = num_blinks * 2;
		s.blink_duration = Clock::MsToTicks(duration_ms) / num_blinks;
		s.set_time = Controls::TimeNow();
	}
	void Set(uint32_t num_blinks, uint32_t duration_ms, uint32_t delay_ms = 0) {
		for (auto i = 0u; i < state.size(); i++) {
			Set(i, num_blinks, duration_ms, delay_ms);
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
	void Update() {
		const auto t = Controls::TimeNow();
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
	uint32_t SettingsVersionTag alignas(4);
	Model::Mode saved_mode alignas(4) = BuildOptions::default_mode;
	Calibration::Dac::Data dac_calibration alignas(4);
	Quantizer::CustomScales custom_scale;

	bool validate() const {
		if (SettingsVersionTag == 0xffffffff) {
			return false;
		}
		if (std::to_underlying(saved_mode) >= Model::ModeMax)
			return false;
		if (!dac_calibration.Validate())
			return false;
		for (auto &s : custom_scale) {
			if (!s.Validate()) {
				return false;
			}
		}
		return true;
	}
};

static_assert(sizeof(Data) % 4 == 0);

class Interface {

public:
	Data &data;
	Interface(Data &data)
		: data{data} {
	}
	Clock::Divider clockdivider;
	DisplayHanger hang;
	Clock::Timer reset{Model::HoldTimes::reset};
	Clock::Timer modeswitcher{Model::HoldTimes::mode_switcher};
	Clock::Timer save{Model::HoldTimes::save};
	Blinker blinker;
	bool do_save_macro = false;
	bool do_save_seq = false;
	bool do_save_shared = false;
	bool did_paste = false;
	bool did_copy = false;
	YoungestSceneButton youngest_scene_button;
	Model::Mode mode = BuildOptions::default_mode;
};

} // namespace Catalyst2::Shared
