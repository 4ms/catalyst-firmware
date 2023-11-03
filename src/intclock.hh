#pragma once
#include "conf/model.hh"
#include <algorithm>

namespace Catalyst2::Clock
{

static constexpr uint32_t ToTicks(uint16_t bpm) {
	return static_cast<unsigned>((60.f * Model::SampleRateHz) / bpm);
}
static constexpr uint16_t ToBpm(uint32_t tick) {
	return static_cast<unsigned>((60.f * Model::SampleRateHz) / tick);
}

class Internal {
	uint32_t timenow = 0;

public:
	uint32_t TimeNow() {
		return timenow;
	}

protected:
	void Update() {
		timenow++;
	}
};

class Bpm : public Internal {
	static constexpr auto sample_rate = Model::SampleRateHz;

	uint32_t cnt = 0;

	uint8_t tap_cnt = 0;
	uint32_t ptaptime = 0;
	uint32_t pulsewidth = 0;

	uint32_t ticks_per_pulse;
	uint16_t bpm;
	bool peek = false;
	bool external = false;
	bool step = false;

	bool multout = false;

public:
	Bpm() {
		Set(120);
	}
	void Update() {
		Internal::Update();
		const auto cntt4 = (cnt % (ticks_per_pulse / 4)) + 1;
		cnt++;

		if (cnt >= ticks_per_pulse) {
			if (IsInternal()) {
				cnt = 0;
				step = true;
			}
			peek = !peek;
		}
		if (cntt4 >= (ticks_per_pulse / 4) || cnt == 0)
			multout = true;
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
		cnt = 0;

		const auto pw = TimeNow() - ptaptime;
		ptaptime = TimeNow();
		Set(ToBpm(pw));
	}
	void Tap() {
		// if this is the first tap in a long time
		// the time between the last tap and this tap is not useful
		if (!IsInternal())
			return;

		if (TimeNow() - ptaptime >= ToTicks(8)) {
			tap_cnt = 0;
			pulsewidth = 0;
			ptaptime = TimeNow();
			return;
		}

		const auto pw = TimeNow() - ptaptime;
		ptaptime = TimeNow();
		pulsewidth += pw;
		tap_cnt += 1;
		Set(ToBpm(pulsewidth / tap_cnt));
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
	void Set(int32_t bpm) {
		this->bpm = std::clamp<int32_t>(bpm, 1, 1200);
		ticks_per_pulse = ToTicks(this->bpm);
	}
	void Inc(int by, bool fine) {
		auto inc = fine ? 1 : 10;
		if (by > 0)
			Set(bpm + inc);
		else if (by < 0)
			Set(bpm - inc);
	}
	float GetPhase() {
		return static_cast<float>(cnt) / ticks_per_pulse;
	}
	void Reset() {
		cnt = 0;
		peek = false;
		tap_cnt = 0;
		pulsewidth = 0;
	}
	bool Times4() {
		const auto out = multout;
		multout = false;
		return out;
	}
};

} // namespace Catalyst2::Clock