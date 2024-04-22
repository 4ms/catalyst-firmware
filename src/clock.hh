#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include <algorithm>
#include <array>
#include <utility>

namespace Catalyst2::Clock
{

constexpr uint32_t BpmToTicks(uint32_t bpm) {
	return (60.f * Model::sample_rate_hz) / bpm;
}
constexpr float TicksToBpm(uint32_t tick) {
	return (60.f * Model::sample_rate_hz) / tick;
}
constexpr uint32_t MsToTicks(uint32_t ms) {
	return (Model::sample_rate_hz / 1000.f) * ms;
}

class Timer {
	const uint32_t duration;
	uint32_t set_time;

public:
	Timer(uint32_t duration_ms)
		: duration{MsToTicks(duration_ms)} {
	}
	void SetAlarm() {
		set_time = Controls::TimeNow();
	}
	bool Check() {
		return Controls::TimeNow() - set_time >= duration;
	}
};

struct Sync {
	enum class Mode : uint8_t {
		SYNCED,
		DIN_SYNC,
	};

	static constexpr auto ModeMax = std::underlying_type_t<Mode>{2};

	Mode mode = Mode::SYNCED;

	void Inc(int32_t inc) {
		const auto temp = std::to_underlying(mode);
		mode = static_cast<Mode>(std::clamp<int32_t>(temp + inc, 0, ModeMax - 1));
	}
	bool Validate() const {
		if (std::to_underlying(mode) >= ModeMax)
			return false;
		return true;
	}
};

namespace Bpm
{
inline constexpr auto min_bpm = 10u;
inline constexpr auto max_bpm = 1200u;

inline constexpr auto max_ticks = BpmToTicks(min_bpm);
inline constexpr auto absolute_max_ticks = INT16_MAX; // ~7.32 bpm
inline constexpr auto min_ticks = BpmToTicks(max_bpm);
inline constexpr auto absolute_min_ticks = 1;

struct Data {
	int16_t bpm_in_ticks = BpmToTicks(120);

	bool Validate() const {
		return bpm_in_ticks >= absolute_min_ticks;
	}
};

class Interface {
	int16_t &bpm_in_ticks;
	uint32_t prev_tap_time;

	uint32_t cnt;
	uint32_t peek_cnt;

	bool did_trig = false;

public:
	bool external;
	bool pause;

	Interface(Data &data)
		: bpm_in_ticks{data.bpm_in_ticks} {
	}

	void Trig() {
		// this wont ever be true unless there is a hardware problem..
		// leaving commented so I don't re-write it and then re-realize it wont be necessary
		// if (!external) {
		//       return;
		// }

		did_trig = !pause;
		cnt = 0;
		peek_cnt = 0;

		Tap();
	}

	float GetBpm() const {
		return Clock::TicksToBpm(bpm_in_ticks);
	}

	bool Update() {
		const uint32_t bpm = bpm_in_ticks;

		peek_cnt++;
		if (peek_cnt >= bpm) {
			peek_cnt = 0;
		}

		cnt += !pause;

		if (external) {
			if (did_trig) {
				did_trig = false;
				return true;
			}
		} else {
			if (cnt >= bpm) {
				cnt = 0;
				peek_cnt = 0;
				return true;
			}
		}

		return false;
	}

	void Inc(int32_t inc, bool fine) {
		auto temp = static_cast<uint32_t>(TicksToBpm(bpm_in_ticks));
		inc = fine ? inc : inc * 10;
		temp = std::clamp<int32_t>(temp + inc, min_bpm, max_bpm);
		bpm_in_ticks = BpmToTicks(temp);
	}

	void Tap() {
		const auto time_now = Controls::TimeNow();
		bpm_in_ticks = std::clamp<uint32_t>(time_now - prev_tap_time, absolute_min_ticks, absolute_max_ticks);
		prev_tap_time = time_now;
	}

	float GetPhase() const {
		const auto tc = static_cast<float>(std::clamp<int32_t>(cnt, 0, bpm_in_ticks - 1));
		return tc / bpm_in_ticks;
	}
	float PeekPhase() const {
		return static_cast<float>(peek_cnt) / bpm_in_ticks;
	}
	void Reset() {
		cnt = 0;
	}
	void ResetPeek() {
		peek_cnt = 0;
	}
};
} // namespace Bpm

class Divider {
	uint32_t counter = 0;

public:
	class type {
		static constexpr auto min = 0u;
		static constexpr auto max = 255u;
		uint8_t v = 0;

	public:
		void Inc(int32_t inc) {
			v = std::clamp<int32_t>(v + inc, min, max);
		}
		uint32_t Read() const {
			return v + 1;
		}
		bool Validate() const {
			// uses entire integer range
			return true;
		}
	};
	bool Update(type div) {
		counter += 1;
		if (counter >= div.Read()) {
			counter = 0;
			return true;
		}
		return false;
	}
	float GetPhase(type div) const {
		return static_cast<float>(counter) / div.Read();
	}
	float GetPhase(type div, float phase) const {
		return (phase / div.Read()) + GetPhase(div);
	}
	void Reset() {
		counter = 0;
	}
};

} // namespace Catalyst2::Clock
