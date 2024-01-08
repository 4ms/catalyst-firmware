#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "quantizer.hh"

namespace Catalyst2::Shared
{

struct Data {
	Clock::Bpm::type bpm{};
	Clock::Divider::type clockdiv{};
	Model::Mode mode = Model::default_mode;
	bool Validate() {
		auto ret = true;
		ret &= clockdiv.Validate();
		ret &= mode == Model::Mode::Macro || mode == Model::Mode::Sequencer;
		ret &= bpm.Validate();
		return ret;
	}
};

class Interface {
	using QuantizerArray = std::array<Quantizer::Interface, Model::NumChans>;
	class DisplayHanger {
		static constexpr uint32_t duration = Clock::MsToTicks(4000);
		uint8_t onto;
		uint32_t start_time;
		Clock::Bpm &internalclock;

	public:
		DisplayHanger(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Cancel() {
			onto = 0xff;
		}
		void Set(uint8_t encoder) {
			start_time = internalclock.TimeNow();
			onto = encoder;
		}
		std::optional<uint8_t> Check() {
			if (internalclock.TimeNow() - start_time >= duration) {
				onto = 0xff;
			}
			if (onto == 0xff) {
				return std::nullopt;
			}
			return onto;
		}
	};
	class ResetManager {
		static constexpr auto hold_duration = Clock::MsToTicks(3000);
		Clock::Bpm &internalclock;
		uint32_t set_time;
		bool notify = false;

	public:
		ResetManager(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Notify(bool on) {
			notify = on;
			if (on) {
				set_time = internalclock.TimeNow();
			}
		}
		bool Check() {
			if (notify == false || internalclock.TimeNow() - set_time < hold_duration) {
				return false;
			}
			return true;
		}
	};
	class ModeSwitcher {
		static constexpr auto hold_duration = Clock::MsToTicks(3000);
		Clock::Bpm &internalclock;
		uint32_t set_time;

	public:
		ModeSwitcher(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Notify() {
			set_time = internalclock.TimeNow();
		}
		bool Check() {
			return internalclock.TimeNow() - set_time >= hold_duration;
		}
	};
	class Blinker {
		Clock::Bpm &internalclock;
		uint32_t remaining = 0;
		uint32_t blink_duration = Clock::MsToTicks(3000);
		uint8_t led = 0;
		uint32_t set_time;

	public:
		Blinker(Clock::Bpm &ic)
			: internalclock{ic} {
		}
		void Set(uint8_t led, uint32_t num_blinks, uint32_t duration_ms) {
			remaining = num_blinks * 2;
			this->led = led;
			blink_duration = Clock::MsToTicks(duration_ms) / num_blinks;
			set_time = internalclock.TimeNow();
		}
		void Update() {
			if (!remaining) {
				return;
			}
			const auto t = internalclock.TimeNow();
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
	Clock::Bpm internalclock{data.bpm};
	QuantizerArray quantizer;
	Clock::Divider clockdivider;
	DisplayHanger hang{internalclock};
	ResetManager reset{internalclock};
	ModeSwitcher modeswitcher{internalclock};
	Blinker blinker{internalclock};
	bool do_save = false;
	bool did_paste = false;
	bool did_copy = false;
	float pos;
};

} // namespace Catalyst2::Shared