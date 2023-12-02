#pragma once

#include "abstract.hh"
#include "params.hh"

namespace Catalyst2::Sequencer::Ui
{
class Usual : public Catalyst2::Ui::Abstract {
public:
	SeqMode::Interface &p;
	Usual(SeqMode::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	void Common() override final {

		if (c.toggle.trig_sense.just_went_high())
			p.shared.internalclock.SetExternal(false);
		else if (c.toggle.trig_sense.just_went_low())
			p.shared.internalclock.SetExternal(true);

		if (c.jack.reset.is_high()) {
			if (c.toggle.trig_sense.is_high())
				p.shared.internalclock.Reset();
			else
				p.shared.clockdivider.Reset();

			p.player.Reset();
			return;
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
		const auto pos = (c.ReadSlider() + c.ReadCv()) / 4095.f;
		p.shared.SetPos(pos);
	}
};
} // namespace Catalyst2::Sequencer::Ui