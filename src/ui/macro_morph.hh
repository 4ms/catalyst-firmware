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
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustPressed(c, [this](uint8_t button) { OnSceneButtonPress(button); });

		if (!c.button.morph.is_high()) {
			return;
		}
		if (c.button.shift.is_high()) {
			return;
		}
		if (!c.button.fine.is_high() && !c.button.play.is_high()) {
			p.shared.modeswitcher.SetAlarm();
		}
		if (p.shared.modeswitcher.Check()) {
			p.shared.mode = Model::Mode::Sequencer;
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.shared.blinker.Set(Model::NumChans - i - 1, 1, 200, 100 * i + 250);
			}
			return;
		}
		interface = this;
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
