#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "helper_functions.hh"
#include "shared.hh"

namespace Catalyst2::Ui
{

inline void ConfirmCopy(Shared::Interface &p, uint8_t led) {
	p.blinker.Set(led, 8, 250);
}
inline void ConfirmPaste(Shared::Interface &p, uint8_t led) {
	ConfirmCopy(p, led);
}
inline void LedBlinker(Controls &c, const Shared::Blinker &b) {
	ClearButtonLeds(c);
	for (auto i = 0u; i < Model::NumChans; i++) {
		c.SetButtonLed(i, b.IsHigh(i));
	}
}

class Abstract {
public:
	Controls &c;
	Abstract(Controls &c)
		: c{c} {
	}

	virtual void Init() {
	}
	virtual void Update(Abstract *&interface) = 0;
	virtual void Common() = 0;
	virtual void PaintLeds(const Model::Output::Buffer &outs) {
	}
};

} // namespace Catalyst2::Ui
