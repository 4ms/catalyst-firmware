#pragma once

#include "conf/model.hh"
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

class Internal {
	uint32_t timenow = 0;

public:
	uint32_t TimeNow() {
		return timenow;
	}

	void Update() {
		timenow++;
	}
};

class Timer {
	const uint32_t duration;
	uint32_t set_time;

public:
	Timer(uint32_t duration_ms)
		: duration{MsToTicks(duration_ms)} {
	}
	void SetAlarm(uint32_t time_now) {
		set_time = time_now;
	}
	bool Check(uint32_t time_now) {
		return time_now - set_time >= duration;
	}
};

namespace Bpm
{
inline constexpr auto min = 8u, max = 1200u;
struct Data {
	uint32_t bpm_in_ticks = BpmToTicks(120u);

	bool Validate() const {
		return bpm_in_ticks <= BpmToTicks(min) && bpm_in_ticks >= BpmToTicks(max);
	}
};

class Interface {
	Data &data;

	uint32_t peek_cnt = 0;
	uint32_t cnt = 0;

	uint32_t prevtaptime;
	bool external = false;
	bool step = false;
	bool peek = false;
	bool pause = false;

public:
	Interface(Data &data)
		: data{data} {
	}
	void Inc(int32_t inc, bool fine) {
		auto temp = TicksToBpm(data.bpm_in_ticks);
		inc = fine ? inc : inc * 10;
		temp = std::clamp<int32_t>(temp + inc, min, max);
		data.bpm_in_ticks = BpmToTicks(temp);
	}
	void Update() {
		const auto period = data.bpm_in_ticks;

		peek_cnt++;
		if (peek_cnt >= period) {
			peek = !peek;
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
	void Input(uint32_t time_now) {
		if (IsInternal()) {
			return;
		}
		step = !pause;

		cnt = 0;

		Tap(time_now);
	}
	void Tap(uint32_t time_now) {
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
		auto out = static_cast<float>(cnt) / data.bpm_in_ticks;
		return std::clamp(out, 0.f, .9999f);
	}
	void Reset() {
		cnt = 0;
	}
	bool Peek() const {
		return peek;
	}
	void Pause() {
		Pause(!pause);
	}
	void Pause(bool pause) {
		this->pause = pause;
	}
	bool IsPaused() const {
		return pause;
	}
};
} // namespace Bpm
class Divider {
	uint32_t counter = 0;
	bool step = false;

public:
	class type {
		static constexpr auto min = 0u, max = 255u;
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
	void Update(type div) {
		counter += 1;
		if (counter >= div.Read()) {
			step = true;
			counter = 0;
		}
	}
	float GetPhase(type div) const {
		return static_cast<float>(counter) / div.Read();
	}
	float GetPhase(type div, float phase) const {
		return (phase / div.Read()) + GetPhase(div);
	}
	bool Step() {
		bool ret = step;
		step = false;
		return ret;
	}
	void Reset() {
		counter = 0;
		step = false;
	}
};

} // namespace Catalyst2::Clock
