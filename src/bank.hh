#pragma once

#include "channel.hh"
#include "channelmode.hh"
#include "conf/model.hh"
#include "random.hh"
#include <algorithm>
#include <array>

namespace Catalyst2::Macro::Bank
{

struct Data {
	struct Scene {
		std::array<Channel::Value, Model::NumChans> channel{};
		Random::Amount::type random_amount;
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
	bool Validate() const {
		auto ret = true;
		for (auto &s : scene) {
			for (auto &cv : s.channel) {
				ret &= cv.Validate();
			}
			ret &= s.random_amount <= Random::Amount::max && s.random_amount >= Random::Amount::min;
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
	Random::Macro::Pool::Interface randompool;

	Interface(Random::Macro::Pool::Data &r)
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
		return b->scene[scene].random_amount;
	}
	void IncRandomAmount(uint8_t scene, int32_t inc) {
		auto t = b->scene[scene].random_amount;
		t += inc * Random::Amount::inc;
		b->scene[scene].random_amount = std::clamp(t, Random::Amount::min, Random::Amount::max);
	}
	void IncChan(uint8_t scene, uint8_t channel, int32_t inc, bool fine) {
		if (GetChannelMode(channel).IsGate()) {
			b->scene[scene].channel[channel].IncGate(inc);
		} else {
			b->scene[scene].channel[channel].IncCv(inc, fine, GetRange(channel));
		}
	}
	float GetMorph(uint8_t channel) {
		return 1.f - b->morph[channel];
	}
	void IncMorph(uint8_t channel, int32_t inc) {
		const auto i = (1.f / 100.f) * inc;
		b->morph[channel] = std::clamp(b->morph[channel] + i, 0.f, 1.f);
	}
	Channel::Value::Proxy GetChannel(uint8_t scene, uint8_t channel) {
		const auto rand = randompool.Read(channel, scene, b->scene[scene].random_amount);
		return b->scene[scene].channel[channel].Read(b->range[channel], rand);
	}
};

} // namespace Catalyst2::Macro::Bank
