#pragma once

#include "abstract.hh"
#include "controls.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Usual : public Catalyst2::Ui::Abstract {
public:
	MacroMode::Interface &p;
	Usual(MacroMode::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	void Common() override final {
		if (c.jack.trig.just_went_high()) {
			if (p.recorder.is_cued()) {
				p.shared.clockdivider.Reset();
				p.recorder.record();
			} else {
				p.shared.clockdivider.Update(p.shared.GetClockDiv());
				if (p.shared.clockdivider.Step())
					p.recorder.reset();
			}
		}
		const auto pos = p.recorder.update(c.ReadSlider() + c.ReadCv()) / 4096.f;
		p.shared.SetPos(pos);
		p.pathway.Update(pos);
		p.override_output = c.YoungestSceneButton();
	}
};

} // namespace Catalyst2::Macro::Ui
