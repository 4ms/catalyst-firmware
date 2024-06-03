#pragma once

#include "abstract.hh"
#include "controls.hh"
#include "params.hh"

namespace Catalyst2::Ui::Macro
{

class Usual : public Abstract {
public:
	Catalyst2::Macro::Interface &p;
	Abstract &main_ui;

	Usual(Catalyst2::Macro::Interface &p, Controls &c, Abstract *main_ui)
		: Abstract{c}
		, p{p}
		, main_ui{*main_ui} {
	}
	void Common() final {
		if (c.jack.trig.just_went_high()) {
			p.Trig();
		}

		PlayButtonDisplay();

		p.Update(c.ReadSlider() + c.ReadCv());
	}

private:
	void PlayButtonDisplay() {
		if (c.button.bank.is_high()) {
			return;
		}
		bool level;
		if (p.recorder.IsCued()) {
			level = (Controls::TimeNow() >> 10) & 1;
		} else {
			level = p.recorder.IsPlaying();
		}
		c.SetPlayLed(level);
	}
};

} // namespace Catalyst2::Ui::Macro
