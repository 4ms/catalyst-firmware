#pragma once
#include "conf/model.hh"

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
		this->bpm = bpm;
		ticks_per_pulse = static_cast<unsigned>(updates_per_minute / bpm);
	}
	void bpm_inc(int by = 1)
	{
		int new_bpm = bpm + by;

		// is there a clamping util?
		if (new_bpm > 300)
			new_bpm = 300;
		if (new_bpm < 30)
			new_bpm = 30;

		set_bpm(new_bpm);
	}

private:
	unsigned bpm;
	unsigned ticks_per_pulse;
	unsigned tick{0};
	bool step_{false};
};

} // namespace Catalyst2