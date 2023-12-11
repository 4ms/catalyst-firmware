#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Bank : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		c.button.morph.clear_events();
	}
	void Update(Abstract *&interface) override {
		if (!c.button.bank.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
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

	void OnSceneButtonRelease(uint8_t button) override {
		p.SelectBank(button);
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedBank(), true);
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto col = p.bank.GetChannelMode(i).GetColor();
			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
