#pragma once
#include "channelmode.hh"
#include "channelvalue.hh"
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
		std::array<Channel, Model::NumChans> channel;
		float random_amount = 1.f / 15.f;
	};
	std::array<Scene, Model::NumScenes> scene;
	std::array<ChannelMode, Model::NumChans> channelmode;
};

class Interface {
	Data::Scene clipboard;
	RandomPool &randompool;
	Data *b;

public:
	Interface(RandomPool &r)
		: randompool{r}
	{}

	void Load(Data &d)
	{
		b = &d;
	}

	void Copy(uint8_t scene)
	{
		clipboard = b->scene[scene];
	}

	void Paste(uint8_t scene)
	{
		b->scene[scene] = clipboard;
	}

	void IncChannelMode(uint8_t channel, int32_t inc)
	{
		b->channelmode[channel].Inc(inc);
	}

	ChannelMode GetChannelMode(uint8_t channel)
	{
		return b->channelmode[channel];
	}

	void SetChanMode(uint8_t channel, ChannelMode mode)
	{
		b->channelmode[channel] = mode;
	}

	float GetRandomAmount(uint8_t scene)
	{
		return b->scene[scene].random_amount;
	}

	void SetRandomAmount(uint8_t scene, float amount)
	{
		b->scene[scene].random_amount = std::clamp(amount, 0.f, 1.f);
	}

	void IncChan(uint8_t scene, uint8_t channel, int32_t inc, bool fine)
	{
		b->scene[scene].channel[channel].Inc(inc, fine, GetChannelMode(channel).IsGate());
	}

	ChannelValue::type GetChannel(uint8_t scene, uint8_t channel)
	{

		auto rand = static_cast<int32_t>(randompool.GetSceneVal(scene, channel) * b->scene[scene].random_amount *
										 ChannelValue::Range);

		if (GetChannelMode(channel).IsGate()) {
			// gates not affected by randomness?
			rand = 0;
		}
		auto temp = b->scene[scene].channel[channel].val + rand;
		return std::clamp<int32_t>(temp, ChannelValue::Min, ChannelValue::Max);
	}
};

} // namespace Catalyst2::Bank
