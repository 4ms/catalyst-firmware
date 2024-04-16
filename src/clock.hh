#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include <algorithm>
#include <array>

namespace Catalyst2::Clock
{

constexpr uint32_t BpmToTicks(uint32_t bpm) {
	return (60.f * Model::sample_rate_hz) / bpm;
}
constexpr uint32_t TicksToBpm(uint32_t tick) {
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

namespace Bpm
{
inline constexpr auto min_bpm = 10u;
inline constexpr auto max_bpm = 1200u;

inline constexpr auto max_ticks = BpmToTicks(min_bpm);
inline constexpr auto min_ticks = BpmToTicks(max_bpm);

enum class Mode : uint8_t {
	FREE_RUN,
	FOLLOW,
	DIN_SYNC,
};

inline constexpr auto ModeMax = std::underlying_type_t<Mode>{3};

struct Data {
	uint32_t bpm_in_ticks = BpmToTicks(120u);

	bool Validate() const {
		return bpm_in_ticks >= min_ticks && bpm_in_ticks <= max_ticks;
	}
};

class Interface {
	Data &data;

	uint32_t peek_cnt = 0;
	uint32_t cnt = 0;

	uint32_t prevtaptime;
	bool external = false;
	bool step = false;
	bool pause = false;
	bool stop = false;

public:
	Interface(Data &data)
		: data{data} {
	}
	void Inc(int32_t inc, bool fine) {
		auto temp = TicksToBpm(data.bpm_in_ticks);
		inc = fine ? inc : inc * 10;
		temp = std::clamp<int32_t>(temp + inc, min_bpm, max_bpm);
		data.bpm_in_ticks = BpmToTicks(temp);
	}
	void Update() {
		const auto period = data.bpm_in_ticks;

		peek_cnt++;
		if (peek_cnt >= period) {
			peek_cnt = 0;
		}

		cnt += !pause;

		if (cnt >= period) {
			if (IsInternal() || cnt >= period * 2) {
				cnt = 0;
				step = true;
				SetExternal(false);
			}
		}
	}
	bool Output() {
		bool ret = step;
		step = false;
		return ret;
	}
	void Trig() {
		if (IsInternal()) {
			return;
		}
		step = !pause;

		cnt = 0;

		Tap();
	}
	void Tap() {
		const auto time_now = Controls::TimeNow();
		data.bpm_in_ticks = time_now - prevtaptime;
		prevtaptime = time_now;
	}
	bool IsInternal() const {
		return external == false;
	}
	void SetExternal(bool on) {
		external = on;
	}
	float GetPhase() const {
		const auto tc = static_cast<float>(std::clamp<int32_t>(cnt, 0, data.bpm_in_ticks - 1));
		return tc / data.bpm_in_ticks;
	}
	float PeekPhase() const {
		auto out = static_cast<float>(peek_cnt) / data.bpm_in_ticks;
		return out;
	}
	void ResetPeek() {
		peek_cnt = 0;
	}
	void Reset() {
		cnt = 0;
	}
	void Pause() {
		Pause(!pause);
	}
	void Pause(bool pause) {
		this->pause = pause;
		stop = false;
	}
	void Stop(bool stop) {
		this->stop = stop;
	}
	bool IsPaused() const {
		return pause;
	}
	bool IsStopped() const {
		return stop;
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
