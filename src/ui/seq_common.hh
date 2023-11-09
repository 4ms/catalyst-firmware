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

			p.seq.player.Reset();
			return;
		}

		if (c.button.play.just_went_high())
			p.seq.player.TogglePause();

		if (c.button.add.just_went_high())
			p.shared.internalclock.Tap();

		const auto pos = (c.ReadSlider() + c.ReadCv()) / 4096.f;
		p.seq.SetMasterPhaseOffset(pos);
	}
};
} // namespace Catalyst2::Sequencer::Ui