#pragma once

#include "channelmode.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "random.hh"
#include "range.hh"
#include "sequence_phaser.hh"
#include "transposer.hh"
#include <algorithm>
#include <array>
#include <optional>

namespace Catalyst2::Sequencer
{
namespace Settings
{
struct PlayMode {
	using type = int8_t;
	enum class Mode : type {
		Forward,
		Backward,
		PingPong,
		Random0,
		Random1,
	};
	static constexpr type min = static_cast<type>(Mode::Forward);
	static constexpr type max = static_cast<type>(Mode::Random1);
	static constexpr type def = min;
	static Mode Read(type val) {
		return static_cast<Mode>(val);
	}
};
struct Length {
	using type = int8_t;
	static constexpr type min = Model::Sequencer::Steps::Min;
	static constexpr type max = Model::Sequencer::Steps::Max;
	static constexpr type def = Model::Sequencer::Steps::PerPage;
};
struct StartOffset {
	using type = int8_t;
	static constexpr type min = Length::min - 1;
	static constexpr type max = Length::max - 1;
	static constexpr type def = min;
};
struct PhaseOffset {
	using type = float;
	static constexpr type min = 0.f;
	static constexpr type max = 1.f;
	static constexpr type def = min;
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

		void Set(T to) {
			val = to;
		}

		T Read() const {
			return val;
		}

		bool Validate() const {
			return val >= min && val <= max;
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
	Setting<Transposer::type> transpose{Transposer::min, Transposer::max, Transposer::def};
	Setting<Random::Amount::type> random{Random::Amount::min, Random::Amount::max, Random::Amount::def};

	bool Validate() const {
		auto ret = true;
		ret &= phaseoffset.Validate();
		ret &= length.Validate();
		ret &= startoffset.Validate();
		ret &= playmode.Validate();
		ret &= transpose.Validate();
		ret &= random.Validate();
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

		void Set(T to, T pivot) {
			val = to;
			above_pivot = val >= pivot;
		}

		std::optional<T> Read() const {
			return val;
		}

		bool Validate() const {
			return val == std::nullopt || (val >= min && val <= max);
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
	Setting<Random::Amount::type> random{Random::Amount::min, Random::Amount::max};
	Catalyst2::Channel::Cv::Range range;
	Catalyst2::Channel::Mode mode;

	bool Validate() const {
		auto ret = true;
		ret &= phaseoffset.Validate();
		ret &= length.Validate();
		ret &= startoffset.Validate();
		ret &= playmode.Validate();
		ret &= transpose.Validate();
		ret &= random.Validate();
		ret &= range.Validate();
		ret &= random.Validate();
		ret &= mode.Validate();
		return ret;
	}
};

class Data {
	std::array<Channel, Model::NumChans> channel;
	Global global;

public:
	Phaser::Data phaser;
	void Clear(uint8_t chan) {
		channel[chan] = Channel{};
	}
	bool Validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.Validate();
		}
		ret &= global.Validate();
		ret &= phaser.Validate();
		return ret;
	}
	const Channel &Copy(uint8_t chan) const {
		return channel[chan];
	}
	void Paste(uint8_t chan, const Channel &d) {
		channel[chan] = d;
	}
	PhaseOffset::type GetPhaseOffsetOrGlobal(std::optional<uint8_t> chan) const {
		return chan.has_value() ? GetPhaseOffset(chan.value()).value_or(GetPhaseOffset()) : GetPhaseOffset();
	}
	Length::type GetLengthOrGlobal(std::optional<uint8_t> chan) const {
		return chan.has_value() ? GetLength(chan.value()).value_or(GetLength()) : GetLength();
	}
	StartOffset::type GetStartOffsetOrGlobal(std::optional<uint8_t> chan) const {
		return chan.has_value() ? GetStartOffset(chan.value()).value_or(GetStartOffset()) : GetStartOffset();
	}
	PlayMode::Mode GetPlayModeOrGlobal(std::optional<uint8_t> chan) const {
		return chan.has_value() ? GetPlayMode(chan.value()).value_or(GetPlayMode()) : GetPlayMode();
	}
	Transposer::type GetTransposeOrGlobal(std::optional<uint8_t> chan) const {
		return chan.has_value() ? GetTranspose(chan.value()).value_or(GetTranspose()) : GetTranspose();
	}
	Random::Amount::type GetRandomOrGlobal(std::optional<uint8_t> chan) const {
		return chan.has_value() ? GetRandom(chan.value()).value_or(GetRandom()) : GetRandom();
	}
	std::optional<PhaseOffset::type> GetPhaseOffset(uint8_t chan) const {
		return channel[chan].phaseoffset.Read();
	}
	PhaseOffset::type GetPhaseOffset() const {
		return global.phaseoffset.Read();
	}
	std::optional<Length::type> GetLength(uint8_t chan) const {
		return channel[chan].length.Read();
	}
	Length::type GetLength() const {
		return global.length.Read();
	}
	std::optional<StartOffset::type> GetStartOffset(uint8_t chan) const {
		return channel[chan].startoffset.Read();
	}
	StartOffset::type GetStartOffset() const {
		return global.startoffset.Read();
	}
	std::optional<PlayMode::Mode> GetPlayMode(uint8_t chan) const {
		if (channel[chan].playmode.Read().has_value()) {
			return PlayMode::Read(channel[chan].playmode.Read().value());
		} else {
			return std::nullopt;
		}
	}
	PlayMode::Mode GetPlayMode() const {
		return PlayMode::Read(global.playmode.Read());
	}
	std::optional<Transposer::type> GetTranspose(uint8_t chan) const {
		return channel[chan].transpose.Read();
	}
	Transposer::type GetTranspose() const {
		return global.transpose.Read();
	}
	std::optional<Random::Amount::type> GetRandom(uint8_t chan) const {
		return channel[chan].random.Read();
	}
	Random::Amount::type GetRandom() const {
		return global.random.Read();
	}
	Catalyst2::Channel::Cv::Range GetRange(uint8_t chan) const {
		return channel[chan].range;
	}
	Clock::Divider::type GetClockDiv(uint8_t chan) {
		return phaser.cdiv[chan];
	}
	Catalyst2::Channel::Mode GetChannelMode(uint8_t chan) const {
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
	void SetStartOffset(uint8_t chan, StartOffset::type val) {
		channel[chan].startoffset.Set(val, global.startoffset.Read());
	}
	void SetStartOffset(StartOffset::type val) {
		global.startoffset.Set(val);
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
	void IncTranspose(uint8_t chan, int32_t inc, bool fine) {
		inc *= fine ? Transposer::inc_step_fine : Transposer::inc_step;
		channel[chan].transpose.Inc(inc, global.transpose.Read());
	}
	void IncTranspose(int32_t inc, bool fine) {
		inc *= fine ? Transposer::inc_step_fine : Transposer::inc_step;
		global.transpose.Inc(inc);
		for (auto &c : channel) {
			c.transpose.UpdatePivot(global.transpose.Read());
		}
	}
	void IncRandom(uint8_t chan, int32_t inc) {
		channel[chan].random.Inc(inc * Random::Amount::inc, global.random.Read());
	}
	void IncRandom(int32_t inc) {
		global.random.Inc(inc * Random::Amount::inc);
		for (auto &c : channel) {
			c.random.UpdatePivot(global.random.Read());
		}
	}
	void IncRange(uint8_t chan, int32_t inc) {
		channel[chan].range.Inc(inc);
	}
	void IncClockDiv(uint8_t chan, int32_t inc) {
		phaser.cdiv[chan].Inc(inc);
	}
	void IncChannelMode(uint8_t chan, int32_t inc) {
		channel[chan].mode.Inc(inc);
	}
	void ToggleMute(uint8_t chan) {
		channel[chan].mode.ToggleMute();
	}
};
} // namespace Settings
inline uint32_t ActualLength(Settings::Length::type length, Settings::PlayMode::Mode pm) {
	if (pm != Settings::PlayMode::Mode::PingPong) {
		return length;
	}
	const auto out = length + length - 2;
	return out < 2 ? 2 : out;
}
} // namespace Catalyst2::Sequencer
