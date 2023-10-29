#pragma once
#include "conf/model.hh"
#include <algorithm>

namespace Catalyst2::Clock
{

class Internal {
	static constexpr auto sample_rate = Model::SampleRateHz;
	static constexpr float updates_per_minute = (60.f * Model::SampleRateHz);

	uint32_t cnt = 0;
	uint32_t ptaptime = 0;
	uint32_t pulsewidth = 0;
	uint32_t ticks_per_pulse;
	uint32_t timenow = 0;
	uint16_t bpm;
	uint8_t tap_cnt = 0;
	bool peek = false;
	bool external = false;
	bool step = false;

public:
	Internal() {
		Set(120);
	}
	void Update() {
		timenow++;

		if (!IsInternal())
			return;

		cnt++;
		if (cnt >= ticks_per_pulse) {
			cnt = 0;
			step = true;
			peek = !peek;
		}
	}
	bool Output() {
		bool ret = step;
		step = false;
		return ret;
	}
	void Input() {
		if (IsInternal())
			return;

		step = true;
		Tap();
	}
	void Tap() {
		const auto pw = timenow - ptaptime;
		ptaptime = timenow;

		if (pw >= ToTicks(20)) {
			tap_cnt = 0;
			pulsewidth = 0;
			return;
		}

		pulsewidth += pw;

		tap_cnt += 1;
		if (tap_cnt == 4) {
			tap_cnt = 0;
			Set(ToBpm(pulsewidth / 4));
			pulsewidth = 0;
		}
	}
	bool IsInternal() {
		return external == false;
	}
	void SetExternal(bool on) {
		external = on;
	}
	bool Peek() {
		return peek;
	}
	void Set(unsigned bpm) {
		this->bpm = std::clamp(bpm, 20u, 1200u);
		ticks_per_pulse = ToTicks(this->bpm);
	}
	void Inc(int by, bool fine) {
		auto inc = fine ? 1 : 10;
		if (by > 0)
			Set(bpm + inc);
		else if (by < 0)
			Set(bpm - inc);
	}
	void Reset() {
		cnt = 0;
		peek = false;
		tap_cnt = 0;
		pulsewidth = 0;
	}

private:
	static constexpr uint32_t ToTicks(uint16_t bpm) {
		return static_cast<unsigned>(updates_per_minute / bpm);
	}
	static constexpr uint16_t ToBpm(uint32_t tick) {
		return static_cast<unsigned>(updates_per_minute / tick);
	}
};

} // namespace Catalyst2::Clock