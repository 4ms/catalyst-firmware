#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "quantizer.hh"
#include <optional>

namespace Catalyst2::Shared
{

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
		uint32_t remaining = 0;
		uint32_t blink_duration = Clock::MsToTicks(3000);
		uint8_t led = 0;
		uint32_t set_time;

	public:
		void Set(uint8_t led, uint32_t num_blinks, uint32_t duration_ms, uint32_t time_now) {
			remaining = num_blinks * 2;
			this->led = led;
			blink_duration = Clock::MsToTicks(duration_ms) / num_blinks;
			set_time = time_now;
		}
		void Update(uint32_t time_now) {
			if (!remaining) {
				return;
			}
			const auto t = time_now;
			if (t - set_time >= blink_duration) {
				set_time = t;
				remaining -= 1;
			}
		}
		bool IsSet() const {
			return remaining > 0;
		}
		bool IsHigh() const {
			return remaining & 0x01;
		}
		uint8_t Led() const {
			return led;
		}
	};

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
