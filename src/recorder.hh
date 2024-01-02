#pragma once

#include "conf/model.hh"
#include "util/math.hh"
#include <array>

namespace Catalyst2::Macro::Recorder
{

struct Data : public std::array<uint16_t, Model::rec_buffer_size> {
	uint32_t length{0};
	bool Validate() const {
		auto ret = true;
		for (auto &i : *this) {
			ret &= i <= 8190;
		}
		ret &= length < Model::rec_buffer_size;
		return ret;
	}
};

class Interface {
	static constexpr auto prescaler = Model::rec_buffer_prescaler;
	static constexpr auto buff_size = Model::rec_buffer_size;
	static constexpr auto max_record_lenth_seconds =
		(1.f / (static_cast<float>(Model::sample_rate_hz) / prescaler)) * buff_size;
	static_assert(MathTools::is_power_of_2(prescaler));

	struct {
		uint8_t recording : 1 = 0;
		uint8_t playing : 1 = 0;
		uint8_t loop_playback : 1 = 0;
		uint8_t cue_rec : 1 = 0;
	} flags;

	unsigned pos_{0};
	uint8_t scaler{0};
	unsigned accum{0};
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}
	uint16_t Update(uint16_t sample) {
		if (!flags.playing && !flags.recording) {
			return sample;
		}
		if (flags.recording) {
			if (!Insert(sample)) {
				Stop();
			}
			return sample;
		}

		return Read();
	}
	void Stop() {
		flags.playing = flags.recording = false;
		scaler = 0;
		accum = 0;
		pos_ = 0;
	}
	void CueRecord() {
		data.length = 0;
		flags.cue_rec = true;
	}
	void Play() {
		flags.playing = true;
	}
	bool IsPlaying() {
		return flags.playing;
	}
	void Record() {
		if (!flags.cue_rec) {
			return;
		}
		flags.cue_rec = false;
		flags.recording = true;
	}
	void Reset() {
		Stop();
		if (flags.cue_rec) {
			flags.cue_rec = false;
			flags.recording = true;
			data.length = 0;
			return;
		}
		if (data.length >= 2) {
			flags.playing = true;
		}
	}
	auto CapacityFilled() {
		return static_cast<float>(data.length) / buff_size;
	}
	auto size() {
		return data.length;
	}
	bool IsRecording() {
		return flags.recording;
	}
	bool IsCued() {
		return flags.cue_rec;
	}

private:
	uint16_t Read() {
		const auto coef = scaler / 16.f;
		const auto out = MathTools::interpolate(data[pos_], data[pos_ + 1], coef);

		if (!scaler) {
			pos_ += 1;
			if (pos_ >= data.length - 1) {
				Stop();
				if (flags.loop_playback) {
					Play();
				}
			}
		}

		scaler++;
		scaler &= prescaler - 1;
		return out;
	}
	bool Insert(uint16_t sample) {
		if (IsFull()) {
			return false;
		}
		accum += sample;
		scaler += 1;
		if (scaler == prescaler) {
			data[data.length++] = accum / prescaler;
			accum = 0;
			scaler = 0;
		}
		return true;
	}
	bool IsFull() {
		return data.length == buff_size;
	}
};
} // namespace Catalyst2::Macro::Recorder