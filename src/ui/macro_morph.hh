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
		if (!c.button.morph.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		p.IncMorph(encoder, inc);

		if (c.button.shift.is_high()) {
			const auto m = p.GetMorph(encoder);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.SetMorph(i, m);
			}
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearEncoderLeds();

		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto morph = p.GetMorph(i);
			auto col = Palette::grey.blend(Palette::red, morph);
			if (morph == 0.f)
				col = Palette::green;

			c.SetEncoderLed(i, col);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
