#pragma once

#include "channel.hh"
#include "channelmode.hh"
#include "conf/model.hh"
#include "pathway.hh"
#include "random.hh"
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>

namespace Catalyst2::Macro
{
namespace Bank
{

struct Data {
	struct Scene {
		std::array<Channel::Value, Model::NumChans> channel{};
		Random::Amount random;
	};
	std::array<Scene, Model::NumScenes> scene{};
	std::array<Channel::Mode, Model::NumChans> channelmode{};
	std::array<Channel::Range, Model::NumChans> range{};
	std::array<float, Model::NumChans> morph{};
	Data() {
		for (auto &m : morph) {
			m = 1.f;
		}
	}
	bool Validate() {
		auto ret = true;
		for (auto &s : scene) {
			ret &= s.random.Validate();
		}
		for (auto &cm : channelmode) {
			ret &= cm.Validate();
		}
		for (auto &r : range) {
			ret &= r.Validate();
		}
		for (auto &m : morph) {
			ret &= (m >= 0.f && m <= 1.f);
		}
		return ret;
	}
};

class Interface {
	Data::Scene clipboard;
	Data *b;

public:
	Random::Pool::Interface<Random::Pool::MacroData> randompool;

	Interface(Random::Pool::MacroData &r)
		: randompool{r} {
	}
	void Load(Data &d) {
		b = &d;
	}
	void Copy(uint8_t scene) {
		clipboard = b->scene[scene];
	}
	void Paste(uint8_t scene) {
		b->scene[scene] = clipboard;
	}
	void IncChannelMode(uint8_t channel, int32_t inc) {
		b->channelmode[channel].Inc(inc);
	}
	void IncRange(uint8_t channel, int32_t inc) {
		b->range[channel].Inc(inc);
	}
	Channel::Range GetRange(uint8_t channel) {
		return b->range[channel];
	}
	Channel::Mode GetChannelMode(uint8_t channel) {
		return b->channelmode[channel];
	}
	void SetChanMode(uint8_t channel, Channel::Mode mode) {
		b->channelmode[channel] = mode;
	}
	auto GetRandomAmount(uint8_t scene) {
		return b->scene[scene].random.Read();
	}
	void IncRandomAmount(uint8_t scene, int32_t inc) {
		b->scene[scene].random.Inc(inc);
	}
	void IncChan(uint8_t scene, uint8_t channel, int32_t inc, bool fine) {
		const auto rand = randompool.Read(channel, scene, b->scene[scene].random);
		b->scene[scene].channel[channel].Inc(inc, fine, GetChannelMode(channel).IsGate(), b->range[channel], rand);
	}
	float GetMorph(uint8_t channel) {
		return 1.f - b->morph[channel];
	}
	void IncMorph(uint8_t channel, int32_t inc) {
		const auto i = (1.f / 100.f) * inc;
		b->morph[channel] = std::clamp(b->morph[channel] + i, 0.f, 1.f);
	}
	Channel::Value::Proxy GetChannel(uint8_t scene, uint8_t channel) {
		const auto rand = randompool.Read(channel, scene, b->scene[scene].random);
		return b->scene[scene].channel[channel].Read(b->range[channel], rand);
	}
};
} // namespace Bank

} // namespace Catalyst2::Macro
