#pragma once

#include "abstract.hh"
#include "params.hh"

namespace Catalyst2::Sequencer::Ui
{
class Usual : public Catalyst2::Ui::Abstract {

public:
	Sequencer::Interface &p;
	Usual(Sequencer::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	void Common() final {
		if (c.jack.reset.just_went_high()) {
			p.Reset();
		}

		if (c.jack.trig.just_went_high()) {
			p.Trig();
		}

		if (!(c.button.add.is_high() && c.button.bank.is_high() && c.button.shift.is_high())) {
			p.shared.modeswitcher.Notify();
		}

		const auto phase = (c.ReadSlider() + c.ReadCv()) / 4095.f;
		p.player.Update(phase);
	}

protected:
	void ConfirmCopy(uint8_t led) {
		p.shared.blinker.Set(led, 8, 250);
	}
	void ConfirmPaste(uint8_t led) {
		ConfirmCopy(led);
	}
	void PlayModeLedAnnimation(Catalyst2::Sequencer::Settings::PlayMode::Mode pm, uint32_t time_now) {
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
};
} // namespace Catalyst2::Sequencer::Ui
