#pragma once

#include "channel.hh"
#include "channelmode.hh"
#include "conf/model.hh"
#include "pathway.hh"
#include "randompool.hh"
#include "util/math.hh"
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>

namespace Catalyst2::Bank
{
struct Data {
	struct Scene {
		std::array<Channel::Value, Model::NumChans> channel;
		float random_amount = 0; // 1.f / 15.f;
	};
	std::array<Scene, Model::NumScenes> scene;
	std::array<Channel::Mode, Model::NumChans> channelmode;
	std::array<Channel::Range, Model::NumChans> range;
	std::array<float, Model::NumChans> morph;
	Data() {
		for (auto &m : morph) {
			m = 1.f;
		}
	}
};

class Interface {
	Data::Scene clipboard;
	RandomPool &randompool;
	Data *b;

public:
	Interface(RandomPool &r)
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
	float GetRandomAmount(uint8_t scene) {
		return b->scene[scene].random_amount;
	}
	void SetRandomAmount(uint8_t scene, float amount) {
		b->scene[scene].random_amount = std::clamp(amount, 0.f, 1.f);
	}
	void IncRandomAmount(uint8_t scene, int32_t inc) {
		auto i = inc / 50.f;
		SetRandomAmount(scene, b->scene[scene].random_amount + i);
	}
	void IncChan(uint8_t scene, uint8_t channel, int32_t inc, bool fine) {
		b->scene[scene].channel[channel].Inc(inc, fine, GetChannelMode(channel).IsGate(), b->range[channel]);
	}
	float GetMorph(uint8_t channel) {
		return b->morph[channel];
	}
	void IncMorph(uint8_t channel, int32_t inc) {
		const auto i = (1.f / 100.f) * inc;
		b->morph[channel] = std::clamp(GetMorph(channel) + i, 0.f, 1.f);
	}
	Model::Output::type GetChannel(uint8_t scene, uint8_t channel) {

		auto rand = static_cast<int32_t>(randompool.GetSceneVal(scene, channel) * b->scene[scene].random_amount *
										 Channel::range);

		if (GetChannelMode(channel).IsGate()) {
			// gates not affected by randomness?
			rand = 0;
		}
		auto temp = b->scene[scene].channel[channel].val + rand;
		return std::clamp<int32_t>(temp, Channel::min, Channel::max);
	}
};

} // namespace Catalyst2::Bank
