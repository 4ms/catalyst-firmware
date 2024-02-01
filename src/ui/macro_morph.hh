#pragma once

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
		if (!c.button.fine.is_high() && !c.button.play.is_high()) {
			p.shared.modeswitcher.Notify(p.shared.internalclock.TimeNow());
		}
		if (p.shared.modeswitcher.Check(p.shared.internalclock.TimeNow())) {
			interface = nullptr;
			p.shared.data.mode = Model::Mode::Sequencer;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.bank.IncMorph(encoder, inc);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds();
		for (auto i = 0u; i < Model::NumChans; i++) {
			auto col = Palette::Morph::color(p.bank.GetMorph(i));
			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
