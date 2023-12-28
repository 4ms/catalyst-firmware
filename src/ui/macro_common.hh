#pragma once

#include "abstract.hh"
#include "controls.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Usual : public Catalyst2::Ui::Abstract {
public:
	Macro::Interface &p;
	Usual(Macro::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	void Common() override final {
		if (c.jack.trig.just_went_high()) {
			if (p.recorder.is_cued()) {
				p.shared.clockdivider.Reset();
				p.recorder.record();
			} else {
				p.shared.clockdivider.Update(p.shared.data.clockdiv);
				if (p.shared.clockdivider.Step()) {
					p.recorder.reset();
				}
				c.SetPlayLed(p.recorder.IsPlaying());
			}
		}

		const auto shift = c.button.shift.is_high();

		if (c.button.play.just_went_high()) {
			if (shift) {
				p.recorder.cue_recording();
			} else {
				p.recorder.reset();
			}
			c.SetPlayLed(p.recorder.IsPlaying());
		}

		if (!(c.button.play.is_high() && c.button.morph.is_high() && c.button.fine.is_high())) {
			p.shared.modeswitcher.Notify();
		}

		const auto pos = p.recorder.update(c.ReadSlider() + c.ReadCv()) / 4095.f;
		p.shared.pos = pos;
		p.pathway.Update(pos);
		p.override_output = YoungestSceneButton();
	}
};

} // namespace Catalyst2::Macro::Ui
