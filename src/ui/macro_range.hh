#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Ui::Macro
{
class Range : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		if (!c.button.morph.is_high() && !c.button.shift.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t channel, int32_t inc) {
		inc = p.shared.hang.Check().has_value() ? inc : 0;
		p.bank.IncRange(channel, inc);
		p.shared.hang.Set(channel);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);
		const auto hang = p.shared.hang.Check();
		if (hang.has_value()) {
			DisplayRange(c, p.bank.GetRange(hang.value()));
			c.SetButtonLed(hang.value(), true);
		} else {
			for (auto i = 0u; i < Model::NumChans; i++) {
				const auto r = p.bank.GetRange(i);
				c.SetEncoderLed(i, Palette::Range::color(r));
			}
		}
	}
};
} // namespace Catalyst2::Ui::Macro
