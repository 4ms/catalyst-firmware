#pragma once

#include "channel.hh"
#include "channelmode.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "randompool.hh"
#include "transposer.hh"
#include "util/countzip.hh"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdint-gcc.h>
#include <type_traits>

namespace Catalyst2::Sequencer
{
using SequenceId = uint8_t;

enum class PlayMode : int8_t {
	Forward,
	Backward,
	PingPong,
	Random,
	NumPlayModes,
};

class StepModifier {
	int8_t m = 0;

public:
	float AsMorph() {
		return m / 8.f;
	}
	uint8_t AsRetrig() {
		auto out = (m + 1) >> 1;
		return std::clamp(out, 0, 3);
	}
	void Inc(int32_t inc) {
		m = std::clamp<int32_t>(m + inc, 0, 8);
	}
};

namespace Settings
{
class Global {
	template<typename T>
	struct Setting {

		Setting(T min, T max, T init = T{})
			: val{init}
			, min{min}
			, max{max} {
		}

		template<typename U>
		void Inc(U inc)
		// TODO: make this work for non-enum and enum types
		//  requires(std::is_convertible_v<T, U>)
		{
			auto int_val = std::clamp<U>(static_cast<U>(val) + inc, static_cast<U>(min), static_cast<U>(max));
			val = T(int_val);
		}

		T Read() {
			return val;
		}

	private:
		T val;
		T min;
		T max;
	};

public:
	Setting<float> phaseoffset{0.f, 1.f, 0.f};
	Setting<int8_t> length{Model::MinSeqSteps, Model::MaxSeqSteps, Model::SeqStepsPerPage};
	Setting<int8_t> startoffset{Model::MinSeqSteps - 1, Model::MaxSeqSteps - 1, 0};
	Setting<PlayMode> playmode{PlayMode::Forward, PlayMode::Random, PlayMode::Forward};
	Setting<Transposer::type> transpose{Transposer::min, Transposer::max, 0};
};

class Channel {
	template<typename T>
	struct Setting {
		Setting(T min, T max)
			: min{min}
			, max{max} {
		}

		template<typename U>
		void Inc(U inc, T pivot)
		// TODO: make this work for non-enum and enum types
		//  requires(std::is_convertible_v<T, U>)
		{
			if (!inc)
				return;

			if (!val.has_value()) {
				auto minmax = min;
				if (inc > 0) {
					above_pivot = true;
					minmax = max;
				} else if (inc < 0)
					above_pivot = false;
				if (pivot == minmax)
					return;
				val = pivot;
				return;
			}
			if (above_pivot) {
				if (static_cast<U>(val.value()) + inc < static_cast<U>(pivot) && val.value() <= pivot) {
					val = std::nullopt;
					return;
				}
			} else {
				if (static_cast<U>(val.value()) + inc > static_cast<U>(pivot) && val.value() <= pivot) {
					val = std::nullopt;
					return;
				}
			}

			auto int_val = std::clamp<U>(static_cast<U>(val.value()) + inc, static_cast<U>(min), static_cast<U>(max));
			val = T(int_val);
		}
		void UpdatePivot(T pivot) {
			if (!val.has_value())
				return;

			if (val.value() > pivot || pivot == min)
				above_pivot = true;
			else if (val.value() < pivot || pivot == max)
				above_pivot = false;
		}

		std::optional<T> Read() {
			return val;
		}

	private:
		std::optional<T> val = std::nullopt;
		bool above_pivot = false;
		T min;
		T max;
	};

public:
	Setting<float> phaseoffset{0.f, 1.f};
	Setting<int8_t> length{Model::MinSeqSteps, Model::MaxSeqSteps};
	Setting<int8_t> startoffset{Model::MinSeqSteps - 1, Model::MaxSeqSteps - 1};
	Setting<PlayMode> playmode{PlayMode::Forward, PlayMode::Random};
	Setting<Transposer::type> transpose{Transposer::min, Transposer::max};
	Catalyst2::Channel::Range range;
	Clock::Divider::type clockdiv;
	float randomamount = 0.f;
	Catalyst2::Channel::Mode mode;
};

class Data {
	std::array<Channel, Model::NumChans> channel;
	Global global;

public:
	const Channel &Copy(uint8_t chan) const {
		return channel[chan];
	}
	void Paste(uint8_t chan, const Channel &d) {
		channel[chan] = d;
	}
	float GetPhaseOffsetOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetPhaseOffset(chan.value()).value_or(GetPhaseOffset()) : GetPhaseOffset();
	}
	int8_t GetLengthOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetLength(chan.value()).value_or(GetLength()) : GetLength();
	}
	int8_t GetStartOffsetOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetStartOffset(chan.value()).value_or(GetStartOffset()) : GetStartOffset();
	}
	PlayMode GetPlayModeOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetPlayMode(chan.value()).value_or(GetPlayMode()) : GetPlayMode();
	}
	Transposer::type GetTransposeOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetTranspose(chan.value()).value_or(GetTranspose()) : GetTranspose();
	}
	std::optional<float> GetPhaseOffset(uint8_t chan) {
		return channel[chan].phaseoffset.Read();
	}
	float GetPhaseOffset() {
		return global.phaseoffset.Read();
	}
	std::optional<int8_t> GetLength(uint8_t chan) {
		return channel[chan].length.Read();
	}
	int8_t GetLength() {
		return global.length.Read();
	}
	std::optional<int8_t> GetStartOffset(uint8_t chan) {
		return channel[chan].startoffset.Read();
	}
	int8_t GetStartOffset() {
		return global.startoffset.Read();
	}
	std::optional<PlayMode> GetPlayMode(uint8_t chan) {
		return channel[chan].playmode.Read();
	}
	PlayMode GetPlayMode() {
		return global.playmode.Read();
	}
	std::optional<Transposer::type> GetTranspose(uint8_t chan) {
		return channel[chan].transpose.Read();
	}
	Transposer::type GetTranspose() {
		return global.transpose.Read();
	}
	Catalyst2::Channel::Range GetRange(uint8_t chan) {
		return channel[chan].range;
	}
	Clock::Divider::type GetClockDiv(uint8_t chan) {
		return channel[chan].clockdiv;
	}
	float GetRandomAmount(uint8_t chan) {
		return channel[chan].randomamount;
	}
	Catalyst2::Channel::Mode GetChannelMode(uint8_t chan) {
		return channel[chan].mode;
	}
	void IncPhaseOffset(uint8_t chan, int32_t inc) {
		auto len = GetLengthOrGlobal(chan);
		len += GetPlayModeOrGlobal(chan) == PlayMode::PingPong ? len - 2 : 0;
		const auto i = static_cast<float>(inc) / len;
		channel[chan].phaseoffset.Inc(i, global.phaseoffset.Read());
	}
	void IncPhaseOffset(int32_t inc) {
		auto len = GetLength();
		len += GetPlayMode() == PlayMode::PingPong ? len - 2 : 0;
		const auto i = static_cast<float>(inc) / len;
		global.phaseoffset.Inc(i);
		for (auto &c : channel) {
			c.phaseoffset.UpdatePivot(global.phaseoffset.Read());
		}
	}
	void IncLength(uint8_t chan, int32_t inc) {
		channel[chan].length.Inc(inc, global.length.Read());
	}
	void IncLength(int32_t inc) {
		global.length.Inc(inc);
		for (auto &c : channel) {
			c.length.UpdatePivot(global.length.Read());
		}
	}
	void IncStartOffset(uint8_t chan, int32_t inc) {
		channel[chan].startoffset.Inc(inc, global.startoffset.Read());
	}
	void IncStartOffset(int32_t inc) {
		global.startoffset.Inc(inc);
		for (auto &c : channel) {
			c.startoffset.UpdatePivot(global.startoffset.Read());
		}
	}
	void IncPlayMode(uint8_t chan, int32_t inc) {
		channel[chan].playmode.Inc(inc, global.playmode.Read());
	}
	void IncPlayMode(int32_t inc) {
		global.playmode.Inc(inc);
		for (auto &c : channel) {
			c.playmode.UpdatePivot(global.playmode.Read());
		}
	}
	void IncTranspose(uint8_t chan, int32_t inc) {
		channel[chan].transpose.Inc(inc, global.transpose.Read());
	}
	void IncTranspose(int32_t inc) {
		global.transpose.Inc(inc);
		for (auto &c : channel) {
			c.transpose.UpdatePivot(global.transpose.Read());
		}
	}
	void IncRange(uint8_t chan, int32_t inc) {
		channel[chan].range.Inc(inc);
	}
	void IncClockDiv(uint8_t chan, int32_t inc) {
		channel[chan].clockdiv.Inc(inc);
	}
	void IncRandomAmount(uint8_t chan, int32_t inc) {
		auto i = (inc / Model::output_octave_range / 12.f);
		channel[chan].randomamount += i;
		channel[chan].randomamount = std::clamp(channel[chan].randomamount, 0.f, 1.f);
	}
	void IncChannelMode(uint8_t chan, int32_t inc) {
		channel[chan].mode.Inc(inc);
	}
	void SetChannelMode(uint8_t chan, Catalyst2::Channel::Mode mode) {
		channel[chan].mode = mode;
	}
};
} // namespace Settings

struct Step : Channel::Value {
	StepModifier modifier;
};

using ChannelData = std::array<Step, Model::MaxSeqSteps>;

class PlayerInterface {
	struct State {
		std::array<uint8_t, Model::MaxSeqSteps> randomstep;
		Clock::Divider clockdivider;
		uint8_t counter = 0;
		uint8_t step = 0;
		uint8_t prev_step = 7;
		bool new_step = false;
	};
	std::array<State, Model::NumChans> channel;
	bool pause = false;
	Settings::Data &d;

public:
	PlayerInterface(Settings::Data &d)
		: d{d} {
	}

	void RandomizeSteps() {
		for (auto i = 0u; i < channel.size(); i++) {
			RandomizeSteps(i);
		}
	}

	void RandomizeSteps(uint8_t chan) {
		// can this be prettier?
		uint32_t *d = reinterpret_cast<uint32_t *>(channel[chan].randomstep.data());
		for (auto i = 0u; i < channel[chan].randomstep.size() / sizeof(uint32_t); i++) {
			d[i] = static_cast<uint32_t>(std::rand());
		}
	}

	void Step() {
		if (pause)
			return;

		for (auto i = 0u; i < channel.size(); i++) {
			Step(i);
		}
	}

	void Reset() {
		for (auto i = 0u; i < channel.size(); i++) {
			Reset(i);
		}
	}
	uint8_t GetFirstStep(std::optional<uint8_t> chan, float phase) {
		return ToStep(chan, 0, phase);
	}

	uint8_t GetPlayheadStep(uint8_t chan, float phase) {
		return ToStep(chan, channel[chan].step, phase);
	}

	uint8_t GetPlayheadStepOnPage(uint8_t chan, float phase) {
		return GetPlayheadStep(chan, phase) % Model::SeqPages;
	}

	uint8_t GetPlayheadPage(uint8_t chan, float phase) {
		return GetPlayheadStep(chan, phase) / Model::SeqPages;
	}
	uint8_t GetPrevStep(uint8_t chan, float phase) {
		return ToStep(chan, channel[chan].prev_step, phase);
	}
	bool IsCurrentStepNew(uint8_t chan) {
		const auto out = channel[chan].new_step;
		channel[chan].new_step = false;
		return out;
	}
	void TogglePause() {
		pause = !pause;
		if (!pause) {
			for (auto &i : channel) {
				i.new_step = true;
			}
		}
	}
	void Stop() {
		pause = true;
		Reset();
	}
	bool IsPaused() {
		return pause;
	}
	float GetPhase(uint8_t chan, float phase) {
		const auto cdiv = d.GetClockDiv(chan);
		phase = phase / cdiv.Read();
		const auto cdivphase = channel[chan].clockdivider.GetPhase(cdiv);
		return phase + cdivphase;
	}

private:
	void Step(uint8_t chan) {
		auto &channel = this->channel[chan];

		channel.clockdivider.Update(d.GetClockDiv(chan));
		if (!channel.clockdivider.Step()) {
			return;
		}
		channel.new_step = true;

		channel.prev_step = channel.step;
		channel.step = channel.counter;
		const auto playmode = d.GetPlayModeOrGlobal(chan);
		const auto length = d.GetLengthOrGlobal(chan);

		channel.counter += 1;
		if (channel.counter >= ActualLength(length, playmode)) {
			channel.counter = 0;
		}
	}
	void Reset(uint8_t chan) {
		auto &c = channel[chan];

		const auto playmode = d.GetPlayModeOrGlobal(chan);
		auto length = d.GetLengthOrGlobal(chan);
		length += playmode == PlayMode::PingPong ? length - 2 : 0;

		c.counter = 0;
		c.clockdivider.Reset();
		c.step = c.counter;
		c.counter += 1;
		c.counter = c.counter >= length ? 0 : c.counter;
		c.prev_step = c.step;
	}

	uint8_t ToStep(std::optional<uint8_t> chan, uint8_t step, float phase) {
		const auto l = d.GetLengthOrGlobal(chan);
		const auto pm = d.GetPlayModeOrGlobal(chan);
		const auto actuallength = ActualLength(l, pm);
		const auto mpo = static_cast<uint32_t>(phase * actuallength);
		auto po = static_cast<uint32_t>(d.GetPhaseOffsetOrGlobal(chan) * actuallength);
		po = po >= actuallength ? actuallength - 1 : po;

		auto s = step + po + mpo;

		switch (pm) {
			using enum PlayMode;
			case Backward:
				s = l + -1 + -s;
				break;
			case Random:
				s = channel[chan.value_or(0)].randomstep[s % l];
				break;
			case PingPong: {
				auto ping = true;
				const auto cmp = l == 1 ? 1u : l - 1u;

				while (s >= cmp) {
					s -= cmp;
					ping = !ping;
				}

				if (!ping) {
					s = l - 1 - s;
				}
				break;
			}
			default:
				break;
		}

		const auto so = d.GetStartOffsetOrGlobal(chan);
		return ((s % l) + so) % Model::MaxSeqSteps;
	}

	uint32_t ActualLength(int8_t length, PlayMode pm) {
		if (pm == PlayMode::PingPong) {
			auto out = length + length - 2;
			if (out < 2) {
				out = 2;
			}
			return out;
		}
		return length;
	}
};

} // namespace Catalyst2::Sequencer
