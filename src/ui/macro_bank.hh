#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"
#include "ui/colors.hh"

namespace Catalyst2::Ui::Macro
{

class Bank : public Usual {
	bool did_save = false;
	MacroColors colors{p, c, main_ui};
	bool touched = false;

public:
	using Usual::Usual;
	void Init() override {
		c.button.play.clear_events();
		c.button.morph.clear_events();
		did_save = false;
		p.shared.colors.SetAlarm();
		touched = false;
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) {
			OnEncoderInc(encoder, inc);
			touched = true;
		});
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) {
			OnSceneButtonRelease(button);
			touched = true;
		});
		if (c.button.play.just_went_low()) {
			p.bank.SelectBank(Model::Macro::Bank::NumNormal);
			touched = true;
		}

		if (c.button.morph.just_went_high()) {
			p.shared.save.SetAlarm();
		}
		if (p.shared.save.Check() && c.button.morph.is_high()) {
			if (!did_save) {
				p.shared.save.SetAlarm();
				p.shared.do_save_macro = true;
				p.shared.blinker.Set(16, 500);
				did_save = true;
			}
		}
		if (p.shared.colors.Check() && !touched) {
			SwitchUiMode(colors);
			return;
		}

		if (!c.button.bank.is_high() && p.shared.youngest_scene_button == std::nullopt) {
			SwitchUiMode(main_ui);
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.IncChannelMode(encoder, inc);
	}

	void OnSceneButtonRelease(uint8_t button) {
		p.bank.SelectBank(button);
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);
		c.SetPlayLed(false);
		if (p.bank.IsBankClassic()) {
			c.SetPlayLed(true);
		} else {
			c.SetButtonLed(p.bank.GetSelectedBank(), true);
		}
		const auto blink = Controls::TimeNow() & (1u << 9);
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto cm = p.bank.GetChannelMode(i);
			if (blink && cm.IsCustomScale()) {
				continue;
			}
			c.SetEncoderLed(i, cm.GetColor());
		}
	}
};
} // namespace Catalyst2::Ui::Macro
