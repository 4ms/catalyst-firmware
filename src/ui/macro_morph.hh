#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"
#include "ui/abstract.hh"

namespace Catalyst2::Ui::Macro
{

class Morph : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.modeswitcher.SetAlarm();
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustPressed(c, [this](uint8_t button) { OnSceneButtonPress(button); });

		if (!c.button.morph.is_high()) {
			SwitchUiMode(main_ui);
			return;
		}
		if (c.button.shift.is_high()) {
			SwitchUiMode(main_ui);
			return;
		}
		if (c.button.fine.is_high()) {
			SwitchUiMode(main_ui);
			return;
		}
	}
	void OnSceneButtonPress(uint8_t button) {
		if (p.bank.IsBankClassic()) {
			p.bank.pathway.ReplaceSceneA(button);
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.bank.IncMorph(encoder, inc);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);
		if (p.bank.IsBankClassic()) {
			c.SetButtonLed(p.bank.pathway.GetSceneA(), true);
		}
		for (auto i = 0u; i < Model::NumChans; i++) {
			auto col = Palette::Morph::color(p.bank.GetMorph(i));
			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Ui::Macro
