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
	uint32_t prevtaptime;
	bool peek = false;
	bool external = false;
	bool step = false;
	bool multout = false;

public:
	class type {
		static constexpr auto min = 1u, max = 1200u;
		uint32_t val;

	public:
		type()
			: val{BpmToTicks(120u)} {
		}
		void Inc(int32_t inc, bool fine) {
			auto temp = TicksToBpm(val);
			inc = fine ? inc : inc * 10;
			temp = std::clamp<int32_t>(temp + inc, min, max);
			val = BpmToTicks(temp);
		}
		uint32_t Read() {
			return val;
		}
		void Set(uint32_t ticks) {
			val = ticks;
		}
		bool Validate() {
			return val <= BpmToTicks(min) && val >= BpmToTicks(max);
		}
	};
	Bpm(type &bpm)
		: bpm{bpm} {
	}
	void Update() {
		Internal::Update();
		const auto period = bpm.Read();
		const auto cntmult = (cnt % (period / multfactor)) + 1;
		cnt++;

		if (cnt >= period) {
			if (IsInternal() || cnt >= period * 2) {
				cnt = 0;
				step = true;
				SetExternal(false);
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
		if (IsInternal()) {
			return;
		}
		step = true;
		cnt = 0;

		Tap();
	}
	void Tap() {
		const auto tn = TimeNow();
		bpm.Set(tn - prevtaptime);
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
	float GetPhase() {
		auto out = static_cast<float>(cnt) / bpm.Read();
		return std::clamp(out, 0.f, 1.f);
	}
	void Reset() {
		cnt = 0;
		peek = false;
	}

private:
	type &bpm;
};

class Divider {
	uint32_t counter = 0;
	bool step = false;

public:
	class type {
		static constexpr std::array<uint8_t, 12> divideroptions = {1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 24, 32};
		uint8_t v = 0;

	public:
		void Inc(int32_t inc) {
			v = std::clamp<int32_t>(v + inc, 0, divideroptions.size() - 1);
		}
		uint8_t Read() const {
			return divideroptions[v];
		}
		bool Validate() const {
			return v < divideroptions.size();
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
