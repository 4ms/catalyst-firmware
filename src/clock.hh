#pragma once

#include "conf/model.hh"
#include <algorithm>
#include <array>

namespace Catalyst2::Clock
{

constexpr uint32_t BpmToTicks(uint32_t bpm) {
	return (60.f * Model::SampleRateHz) / bpm;
}
constexpr uint32_t TicksToBpm(uint32_t tick) {
	return (60.f * Model::SampleRateHz) / tick;
}
constexpr uint32_t MsToTicks(uint32_t ms) {
	return (Model::SampleRateHz / 1000.f) * ms;
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
	uint32_t cnt = 0;
	uint32_t period;
	uint32_t bpm;
	uint32_t prevtaptime;
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
		const auto cntt12 = (cnt % (period / 12)) + 1;
		cnt++;

		if (cnt >= period) {
			if (IsInternal()) {
				cnt = 0;
				step = true;
			}
			peek = !peek;
		}
		if (cntt12 >= (period / 12) || cnt == 0)
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
		return static_cast<float>(cnt) / period;
	}
	void Reset() {
		cnt = 0;
		peek = false;
	}
	bool Times12() {
		const auto out = multout;
		multout = false;
		return out;
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
