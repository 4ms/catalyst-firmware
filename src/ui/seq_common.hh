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
				Stop();
				p.shared.reset.Notify(true);
			} else {
				p.shared.internalclock.Pause();
			}
			c.SetPlayLed(!p.shared.internalclock.IsPaused());
		}

		if (c.button.play.just_went_low()) {
			p.shared.reset.Notify(false);
		}

		if (!(c.button.add.is_high() && c.button.bank.is_high() && c.button.shift.is_high())) {
			p.shared.modeswitcher.Notify();
		}

		const auto cp = c.ReadCv() / 4096.f;
		const auto sp = c.ReadSlider() / 4096.f;
		p.player.Update(sp + cp, p.shared.internalclock.GetPhase(), p.shared.internalclock.Output());
	}
	void Stop() {
		p.shared.clockdivider.Reset();
		p.shared.internalclock.Reset();
		p.player.Reset();
		p.shared.internalclock.Pause(true);
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