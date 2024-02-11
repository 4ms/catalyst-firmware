#pragma once

#include "abstract.hh"
#include "params.hh"

namespace Catalyst2::Ui::Sequencer
{
inline void PlayModeLedAnnimation(Controls &c, Catalyst2::Sequencer::Settings::PlayMode::Mode pm, uint32_t time_now) {
	using namespace Palette::Setting;
	static constexpr auto animation_duration = static_cast<float>(Clock::MsToTicks(1000));
	auto phase = (time_now / animation_duration);
	phase -= static_cast<uint32_t>(phase);
	Color col;

	using enum Catalyst2::Sequencer::Settings::PlayMode::Mode;

	if (pm == Forward) {
		col = playmode_fwd.blend(Palette::off, 1.f - phase);
	} else if (pm == Backward) {
		col = playmode_bck.blend(Palette::off, phase);
	} else if (pm == PingPong) {
		if (phase < .5f) {
			phase *= 2.f;
			col = playmode_bck.blend(Palette::off, phase);
		} else {
			phase -= .5f;
			phase *= 2.f;
			col = playmode_fwd.blend(Palette::off, 1.f - phase);
		}
	} else {
		col = Palette::Random::color(time_now >> 8);
	}
	c.SetEncoderLed(Model::SeqEncoderAlts::PlayMode, col);
}

class Usual : public Abstract {

public:
	Catalyst2::Sequencer::Interface &p;
	Usual(Catalyst2::Sequencer::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	void Common() final {
		p.seqclock.Update();

		if (c.jack.reset.just_went_high()) {
			p.Reset(false);
		}

		if (c.jack.trig.just_went_high()) {
			p.Trig();
		}

		const auto phase = (c.ReadSlider() + c.ReadCv()) / 4095.f;
		p.Update(phase);
	}

protected:
	void BlinkSelectedPage(uint8_t page) {
		c.SetButtonLed(page, ((p.shared.internalclock.TimeNow() >> 8) & 1) > 0);
	}
	void SetPlayheadLed() {
		c.SetEncoderLed(p.player.GetPlayheadStepOnPage(p.GetSelectedChannel()), Palette::SeqHead::color);
	}
};
} // namespace Catalyst2::Ui::Sequencer
