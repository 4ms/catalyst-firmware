#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Morph : public Usual {
public:
	using Usual::Usual;
	void Update(Abstract *&interface) override {
		if (!c.button.morph.is_high()) {
			return;
		}
		if (c.button.shift.is_high()) {
			return;
		}
		if (p.shared.modeswitcher.Check()) {
			interface = nullptr;
			p.shared.data.mode = Model::Mode::Sequencer;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		p.bank.IncMorph(encoder, inc);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds();
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto col =
				p.bank.GetMorph(i) == 0.f ? Palette::green : Palette::grey.blend(Palette::red, p.bank.GetMorph(i));
			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
