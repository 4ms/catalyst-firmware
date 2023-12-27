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
		if (c.jack.reset.is_high()) {
			p.shared.internalclock.Reset();
			p.shared.clockdivider.Reset();
			p.player.Reset();
			return;
		}

		if (c.jack.trig.just_went_high()) {
			if (p.shared.internalclock.IsInternal()) {
				p.shared.internalclock.SetExternal(true);
			}
			p.shared.clockdivider.Update(p.shared.data.clockdiv);
			if (p.shared.clockdivider.Step()) {
				p.shared.internalclock.Input();
			}
		}

		if (c.button.play.just_went_high()) {
			if (c.button.shift.is_high()) {
				p.player.Stop();
				p.shared.reset.Notify(true);
			} else {
				p.player.TogglePause();
			}
		}

		if (c.button.play.just_went_low()) {
			p.shared.reset.Notify(false);
		}

		if (c.button.add.just_went_high()) {
			p.shared.internalclock.Tap();
		}

		if (!(c.button.add.is_high() && c.button.bank.is_high() && c.button.shift.is_high())) {
			p.shared.modeswitcher.Notify();
		}

		const auto phase = (c.ReadSlider() + c.ReadCv()) / 4095.f;
		p.player.Update(phase);
	}
};
} // namespace Catalyst2::Sequencer::Ui