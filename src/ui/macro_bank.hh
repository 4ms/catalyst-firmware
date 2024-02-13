#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Ui::Macro
{

class Bank : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		c.button.play.clear_events();
		c.button.morph.clear_events();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });
		if (c.button.play.just_went_low()) {
			p.SelectBank(Model::NumNormalBanks);
		}

		if (c.button.morph.just_went_high()) {
			p.shared.save.SetAlarm(p.shared.internalclock.TimeNow());
		}
		if (p.shared.save.Check(p.shared.internalclock.TimeNow()) && c.button.morph.is_high()) {
			p.shared.save.SetAlarm(p.shared.internalclock.TimeNow());
			p.shared.do_save = true;
			p.shared.blinker.Set(16, 500, p.shared.internalclock.TimeNow());
		}

		if (!c.button.bank.is_high() && p.shared.youngest_scene_button == std::nullopt) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		if (c.button.shift.is_high()) {
			// change all channgels.
			auto cm = p.bank.GetChannelMode(encoder);
			cm.Inc(inc);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.bank.SetChanMode(i, cm);
				p.shared.quantizer[i].Load(cm.GetScale());
			}
		} else {
			p.bank.IncChannelMode(encoder, inc);
			p.shared.quantizer[encoder].Load(p.bank.GetChannelMode(encoder).GetScale());
		}
	}

	void OnSceneButtonRelease(uint8_t button) {
		p.SelectBank(button);
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		c.SetPlayLed(false);
		if (p.bank.IsBankClassic()) {
			c.SetPlayLed(true);
		} else {
			c.SetButtonLed(p.bank.GetSelectedBank(), true);
		}
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto col = p.bank.GetChannelMode(i).GetColor();
			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Ui::Macro
