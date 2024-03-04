#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Ui::Macro
{

class Reset : public Usual {
	bool wait = true;

public:
	using Usual::Usual;
	void Init() override {
		wait = true;
		p.recorder.Stop();
	}
	void Update() override {
		if (wait) {
			if (!c.button.shift.is_high() && !c.button.play.is_high()) {
				wait = false;
			}
		} else {
			if (c.button.play.is_high() || c.button.fine.is_high() || c.button.morph.is_high() ||
				c.button.bank.is_high() || c.button.shift.is_high())
			{
				SwitchUiMode(main_ui);
				return;
			}
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high()) {
					p.bank.ClearScene(i);
					SwitchUiMode(main_ui);
					return;
				}
			}
			if (c.button.add.is_high()) {
				p.bank.Clear();
				SwitchUiMode(main_ui);
				return;
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		if ((p.shared.internalclock.TimeNow() >> 10u) & 0x01) {
			SetButtonLedsCount(c, Model::NumChans, true);
		}
	}
};
} // namespace Catalyst2::Ui::Macro
