#pragma once
#include "conf/model.hh"
#include <algorithm>

namespace Catalyst2
{

template<unsigned update_freq_hz, unsigned ppqn = 1>
class InternalClock {
	static constexpr float updates_per_minute = (60.f * update_freq_hz) / ppqn;

public:
	InternalClock()
	{
		set_bpm(120);
	}

	void update()
	{
		tick++;
		if (tick >= ticks_per_pulse) {
			tick = 0;
			step_ = true;
		}
	}
	bool step()
	{
		bool ret = step_;
		step_ = false;
		return ret;
	}
	void set_bpm(unsigned bpm)
	{
		this->bpm = std::clamp(bpm, 30u, 1200u);
		ticks_per_pulse = static_cast<unsigned>(updates_per_minute / bpm);
	}
	void bpm_inc(int by, bool fine = false)
	{
		auto inc = fine ? 1 : 10;
		if (by > 0)
			set_bpm(bpm + inc);
		else if (by < 0)
			set_bpm(bpm - inc);
	}

private:
	uint16_t bpm;
	unsigned ticks_per_pulse;
	unsigned tick{0};
	bool step_{false};
};

} // namespace Catalyst2