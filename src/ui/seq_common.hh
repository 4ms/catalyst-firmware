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
	void Common() override final {
		if (c.jack.reset.just_went_high()) {
			p.Reset();
		}

		if (c.jack.trig.just_went_high()) {
			p.Trig();
		}

		if (c.button.play.just_went_high()) {
			if (c.button.shift.is_high()) {
				p.player.Stop();
				p.shared.reset.Notify(true);
			} else {
				p.player.TogglePause();
			}
			c.SetPlayLed(!p.player.IsPaused());
		}

		if (c.button.play.just_went_low()) {
			p.shared.reset.Notify(false);
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
};
} // namespace Catalyst2::Sequencer::Ui