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
	void Common() final {
		if (c.jack.trig.just_went_high()) {
			if (p.recorder.IsCued()) {
				p.shared.clockdivider.Reset();
				p.recorder.Record();
			} else {
				p.shared.clockdivider.Update(p.GetClockDiv());
				if (p.shared.clockdivider.Step()) {
					p.recorder.Reset();
				}
				c.SetPlayLed(p.recorder.IsPlaying());
			}
		}

		const auto shift = c.button.shift.is_high();

		if (c.button.play.just_went_high()) {
			if (shift) {
				p.recorder.CueRecord();
			} else {
				p.recorder.Reset();
			}
			c.SetPlayLed(p.recorder.IsPlaying());
		}

		auto pos = p.recorder.Update(c.ReadSlider() + c.ReadCv()) / 4095.f;
		pos = p.slider_slew.Update(pos);
		p.shared.pos = pos;
		p.pathway.Update(pos);
		p.override_output = YoungestSceneButton();
	}
};

} // namespace Catalyst2::Macro::Ui
