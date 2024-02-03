#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Morph : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.modeswitcher.Notify(p.shared.internalclock.TimeNow());
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		if (!c.button.morph.is_high()) {
			return;
		}
		if (c.button.shift.is_high()) {
			return;
		}
		if (!c.button.fine.is_high() && !c.button.play.is_high()) {
			p.shared.modeswitcher.Notify(p.shared.internalclock.TimeNow());
		}
		if (p.shared.modeswitcher.Check(p.shared.internalclock.TimeNow())) {
			p.shared.data.mode = Model::Mode::Sequencer;
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.shared.blinker.Set(Model::NumChans - i - 1, 1, 200, p.shared.internalclock.TimeNow(), 100 * i + 250);
			}
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.bank.IncMorph(encoder, inc);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();
		for (auto i = 0u; i < Model::NumChans; i++) {
			auto col = Palette::Morph::color(p.bank.GetMorph(i));
			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
