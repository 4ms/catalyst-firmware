#pragma once

#include "channelmode.hh"
#include "conf/model.hh"
#include "random.hh"
#include "transposer.hh"
#include <algorithm>
#include <array>
#include <optional>

namespace Catalyst2::Sequencer::Settings
{

struct PlayMode {
	using type = int8_t;
	enum class Mode : type {
		Forward,
		Backward,
		PingPong,
		Random,
	};
	static constexpr type min = static_cast<type>(Mode::Forward), max = static_cast<type>(Mode::Random), def = min;
	static Mode Read(type val) {
		return static_cast<Mode>(val);
	}
};
struct Length {
	using type = int8_t;
	static constexpr type min = Model::MinSeqSteps, max = Model::MaxSeqSteps, def = Model::SeqStepsPerPage;
};
struct StartOffset {
	using type = int8_t;
	static constexpr type min = Length::min - 1, max = Length::max - 1, def = min;
};
struct PhaseOffset {
	using type = float;
	static constexpr type min = 0.f, max = 1.f, def = min;
};

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

		bool Validate() {
			return val >= min || val <= max;
		}

	private:
		T val;
		T min;
		T max;
	};

public:
	Setting<PhaseOffset::type> phaseoffset{PhaseOffset::min, PhaseOffset::max, PhaseOffset::def};
	Setting<Length::type> length{Length::min, Length::max, Length::def};
	Setting<StartOffset::type> startoffset{StartOffset::min, StartOffset::max, StartOffset::def};
	Setting<PlayMode::type> playmode{PlayMode::min, PlayMode::max, PlayMode::def};
	Setting<Transposer::type> transpose{Transposer::min, Transposer::max, 0};
	bool Validate() {
		auto ret = true;
		ret &= phaseoffset.Validate();
		ret &= length.Validate();
		ret &= startoffset.Validate();
		ret &= playmode.Validate();
		ret &= transpose.Validate();
		return ret;
	}
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

		bool Validate() {
			return val == std::nullopt || val >= min || val <= max;
		}

	private:
		std::optional<T> val = std::nullopt;
		bool above_pivot = false;
		T min;
		T max;
	};

public:
	Setting<PhaseOffset::type> phaseoffset{PhaseOffset::min, PhaseOffset::max};
	Setting<Length::type> length{Length::min, Length::max};
	Setting<StartOffset::type> startoffset{StartOffset::min, StartOffset::max};
	Setting<PlayMode::type> playmode{PlayMode::min, PlayMode::max};
	Setting<Transposer::type> transpose{Transposer::min, Transposer::max};
	Catalyst2::Channel::Range range;
	Clock::Divider::type clockdiv;
	Random::Amount random;
	Catalyst2::Channel::Mode mode;

	bool Validate() {
		auto ret = true;
		ret &= phaseoffset.Validate();
		ret &= length.Validate();
		ret &= startoffset.Validate();
		ret &= playmode.Validate();
		ret &= transpose.Validate();
		ret &= range.Validate();
		ret &= clockdiv.Validate();
		ret &= random.Validate();
		ret &= mode.Validate();
		return ret;
	}
};

class Data {
	std::array<Channel, Model::NumChans> channel;
	Global global;

public:
	void Clear(uint8_t chan) {
		channel[chan] = Channel{};
	}
	bool Validate() {
		for (auto &c : channel) {
			if (!c.Validate()) {
				return false;
			}
		}
		return global.Validate();
	}
	const Channel &Copy(uint8_t chan) const {
		return channel[chan];
	}
	void Paste(uint8_t chan, const Channel &d) {
		channel[chan] = d;
	}
	PhaseOffset::type GetPhaseOffsetOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetPhaseOffset(chan.value()).value_or(GetPhaseOffset()) : GetPhaseOffset();
	}
	Length::type GetLengthOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetLength(chan.value()).value_or(GetLength()) : GetLength();
	}
	StartOffset::type GetStartOffsetOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetStartOffset(chan.value()).value_or(GetStartOffset()) : GetStartOffset();
	}
	PlayMode::Mode GetPlayModeOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetPlayMode(chan.value()).value_or(GetPlayMode()) : GetPlayMode();
	}
	Transposer::type GetTransposeOrGlobal(std::optional<uint8_t> chan) {
		return chan.has_value() ? GetTranspose(chan.value()).value_or(GetTranspose()) : GetTranspose();
	}
	std::optional<PhaseOffset::type> GetPhaseOffset(uint8_t chan) {
		return channel[chan].phaseoffset.Read();
	}
	PhaseOffset::type GetPhaseOffset() {
		return global.phaseoffset.Read();
	}
	std::optional<Length::type> GetLength(uint8_t chan) {
		return channel[chan].length.Read();
	}
	Length::type GetLength() {
		return global.length.Read();
	}
	std::optional<StartOffset::type> GetStartOffset(uint8_t chan) {
		return channel[chan].startoffset.Read();
	}
	StartOffset::type GetStartOffset() {
		return global.startoffset.Read();
	}
	std::optional<PlayMode::Mode> GetPlayMode(uint8_t chan) {
		if (channel[chan].playmode.Read().has_value()) {
			return PlayMode::Read(channel[chan].playmode.Read().value());
		} else {
			return std::nullopt;
		}
	}
	PlayMode::Mode GetPlayMode() {
		return PlayMode::Read(global.playmode.Read());
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
	Random::Amount GetRandomAmount(uint8_t chan) {
		return channel[chan].random;
	}
	Catalyst2::Channel::Mode GetChannelMode(uint8_t chan) {
		return channel[chan].mode;
	}
	void IncPhaseOffset(uint8_t chan, int32_t inc) {
		using enum PlayMode::Mode;
		auto len = GetLengthOrGlobal(chan);
		len += GetPlayModeOrGlobal(chan) == PingPong ? len - 2 : 0;
		const auto i = static_cast<float>(inc) / len;
		channel[chan].phaseoffset.Inc(i, global.phaseoffset.Read());
	}
	void IncPhaseOffset(int32_t inc) {
		using enum PlayMode::Mode;
		auto len = GetLength();
		len += GetPlayMode() == PingPong ? len - 2 : 0;
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
		channel[chan].random.Inc(inc);
	}
	void IncChannelMode(uint8_t chan, int32_t inc) {
		channel[chan].mode.Inc(inc);
	}
	void SetChannelMode(uint8_t chan, Catalyst2::Channel::Mode mode) {
		channel[chan].mode = mode;
	}
};
} // namespace Catalyst2::Sequencer::Settings