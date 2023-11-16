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

protected:
	void Update() {
		timenow++;
	}
};

// Eloquencer can do BPM of 350 max, ratchet x 3 max -> 14.2ms pulses
// Step period (no ratchet), mean 42.8ms = 23.3Hz
class Bpm : public Internal {
	static constexpr auto multfactor = Model::clock_mult_factor;
	uint32_t cnt = 0;
	uint32_t bpm = 120;
	uint32_t period = BpmToTicks(120);
	uint32_t prevtaptime;
	bool peek = false;
	bool external = false;
	bool step = false;
	bool multout = false;

public:
	void Update() {
		Internal::Update();
		const auto cntmult = (cnt % (period / multfactor)) + 1;
		cnt++;

		if (cnt >= period) {
			if (IsInternal()) {
				cnt = 0;
				step = true;
			}
			peek = !peek;
			multout = true;
		}
		if (cntmult >= period / multfactor) {
			multout = true;
		}
	}

	bool Output() {
		bool ret = step;
		step = false;
		return ret;
	}
	bool MultOutput() {
		const auto out = multout;
		multout = false;
		return out;
	}
	void Input() {
		if (IsInternal())
			return;

		step = true;
		cnt = 0;

		Tap();
	}
	void Tap() {
		const auto tn = TimeNow();
		Set(TicksToBpm(tn - prevtaptime));
		prevtaptime = tn;
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
		period = BpmToTicks(this->bpm);
	}
	void Inc(int by, bool fine) {
		auto inc = fine ? 1 : 10;
		if (by > 0)
			Set(bpm + inc);
		else if (by < 0)
			Set(bpm - inc);
	}
	float GetPhase() {
		auto out = static_cast<float>(cnt) / period;
		return std::clamp(out, 0.f, 1.f);
	}
	void Reset() {
		cnt = 0;
		peek = false;
	}
};

class Divider {
	uint32_t counter = 0;
	bool step = false;

public:
	class type {
		static constexpr std::array<uint8_t, 12> divideroptions = {1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32};
		int32_t v = 0;

	public:
		void Inc(int32_t inc) {
			v = std::clamp<int32_t>(v + inc, 0, divideroptions.size() - 1);
		}
		uint8_t Read() {
			return divideroptions[v];
		}
	};
	void Update(type div) {
		counter += 1;
		if (counter >= div.Read()) {
			step = true;
			counter = 0;
		}
	}
	float GetPhase(type div) {
		return static_cast<float>(counter) / div.Read();
	}
	bool Step() {
		bool ret = false;
		if (step) {
			ret = true;
			step = false;
		}
		return ret;
	}
	void Reset() {
		counter = 0;
		step = false;
	}
};

} // namespace Catalyst2::Clock
