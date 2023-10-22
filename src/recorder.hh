#pragma once
#include "util/math.hh"
#include <array>

namespace Catalyst2
{

class Recorder {
	static constexpr auto prescaler = Model::rec_buffer_prescaler;
	static constexpr auto buff_size = Model::rec_buffer_size;
	static constexpr auto max_record_lenth_seconds = (1.f / (1000.f / prescaler)) * buff_size;
	static_assert(MathTools::is_power_of_2(prescaler));

	struct {
		uint8_t recording : 1 = 0;
		uint8_t playing : 1 = 0;
		uint8_t loop_playback : 1 = 0;
		uint8_t cue_rec : 1 = 0;
	} flags;

	std::array<uint16_t, buff_size> buffer;
	unsigned size_{0};
	unsigned pos_{0};
	uint8_t scaler{0};
	unsigned accum{0};

public:
	uint16_t update(uint16_t sample)
	{
		if (!flags.playing && !flags.recording)
			return sample;

		if (flags.recording) {
			if (!insert(sample)) {
				stop();
			}
			return sample;
		}

		return read();
	}
	void stop()
	{
		flags.playing = flags.recording = false;
		scaler = 0;
		accum = 0;
		pos_ = 0;
	}
	void cue_recording()
	{
		size_ = 0;
		flags.cue_rec = true;
	}
	void play()
	{
		flags.playing = true;
	}
	void record()
	{
		if (!flags.cue_rec)
			return;

		flags.cue_rec = false;
		flags.recording = true;
	}
	void reset()
	{
		stop();
		if (flags.cue_rec) {
			flags.cue_rec = false;
			flags.recording = true;
			size_ = 0;
			return;
		}
		if (size_ >= 2)
			flags.playing = true;
	}
	void toggle_loop()
	{
		flags.loop_playback ^= 1;
	}
	auto capacity_filled()
	{
		return static_cast<float>(size_) / buff_size;
	}
	auto size()
	{
		return size_;
	}
	bool is_recordering()
	{
		return flags.recording;
	}
	bool is_cued()
	{
		return flags.cue_rec;
	}

private:
	uint16_t read()
	{
		const auto coef = scaler / 16.f;
		const auto out = MathTools::interpolate(buffer[pos_], buffer[pos_ + 1], coef);

		if (!scaler) {
			pos_ += 1;
			if (pos_ >= size_ - 1) {
				stop();
				if (flags.loop_playback)
					play();
			}
		}

		scaler++;
		scaler &= prescaler - 1;
		return out;
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