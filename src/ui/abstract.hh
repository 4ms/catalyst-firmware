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
	Abstract(Controls &c, Abstract &main_ui)
		: c{c}
		, main_ui{main_ui} {
	}

	virtual void Init() {
	}
	virtual void Update() = 0;
	virtual void Common() = 0;
	virtual void PaintLeds(const Model::Output::Buffer &outs) {
	}

	std::optional<Abstract *> NextUi() {
		auto next = next_ui;
		next_ui = same_ui_tag;
		return next;
	}

protected:
	Abstract &main_ui;
	void SwitchUiMode(Abstract &next) {
		next_ui = &next;
	}

private:
	std::optional<Abstract *> next_ui = same_ui_tag;
	static constexpr std::optional<Abstract *> same_ui_tag = std::nullopt;
};

} // namespace Catalyst2::Ui
