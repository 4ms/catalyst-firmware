#pragma once
#include "util/math.hh"
#include <array>

namespace Catalyst2
{

class Recorder {
	static constexpr auto prescaler = Model::rec_buffer_prescaler;
	static constexpr auto buff_size = Model::rec_buffer_size;
	static constexpr auto max_record_lenth_seconds = (1.f / ((float)Board::cv_stream_hz / prescaler)) * buff_size;
	static_assert(MathTools::is_power_of_2(prescaler));

	std::array<uint16_t, buff_size> buffer;
	bool recording = false;
	bool play_ = false;
	unsigned size_ = 0;
	unsigned pos_ = 0;
	uint8_t scaler = 0;
	unsigned accum = 0;

public:
	uint16_t update(uint16_t sample)
	{
		if (!play_ && !recording)
			return sample;

		if (recording) {
			if (!insert(sample)) {
				// buffer full.
				stop();
			}
			return sample;
		}

		return read();
	}
	void play()
	{
		play_ = true;
	}
	void toggle()
	{
		if (play_)
			stop();
		else
			restart();
	}
	void pause()
	{
		play_ = false;
	}
	void stop()
	{
		pause();
		recording = false;
		scaler = 0;
		pos_ = 0;
	}
	void restart()
	{
		stop();
		play();
	}
	void clear()
	{
		size_ = 0;
	}
	void record()
	{
		stop();
		clear();
		accum = 0;
		recording = true;
	}
	auto size()
	{
		return size_;
	}
	auto capacity_filled()
	{
		return static_cast<float>(size_) / buff_size;
	}
	bool is_recordering()
	{
		return recording;
	}

private:
	// TODO: interpolate?? YES
	uint16_t read()
	{
		if (!scaler) {
			pos_ += 1;
			if (pos_ >= size_) {
				pos_ = 0;
			}
		}
		scaler++;
		scaler &= prescaler - 1;
		return buffer[pos_];
	}
	bool insert(uint16_t sample)
	{
		if (is_full())
			return false;

		accum += sample;
		scaler += 1;
		if (scaler == prescaler) {
			buffer[size_++] = accum / prescaler;
			accum = 0;
			scaler = 0;
		}
		return true;
	}
	bool is_full()
	{
		return size_ == buff_size;
	}
	bool has_space()
	{
		return !is_full();
	}
};
} // namespace Catalyst2