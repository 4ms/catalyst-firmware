#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{
class Range : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
	}
	void Update(Abstract *&interface) override {
		if (!c.button.morph.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t channel, int32_t inc) override {
		inc = p.shared.hang.Check().has_value() ? inc : 0;
		p.bank.IncRange(channel, inc);
		p.shared.hang.Set(channel);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();
		const auto hang = p.shared.hang.Check();
		if (hang.has_value()) {
			DisplayRange(p.bank.GetRange(hang.value()));
			c.SetButtonLed(hang.value(), true);
		} else {
			for (auto i = 0u; i < Model::NumChans; i++) {
				const auto r = p.bank.GetRange(i);
				const auto phase = (r.PosAmount() + r.NegAmount()) / 1.5f;
				const auto blend = phase < .25f ? std::array{Palette::off, Palette::red} :
								   phase < .5f	? std::array{Palette::red, Palette::yellow} :
								   phase < .75f ? std::array{Palette::yellow, Palette::green} :
												  std::array{Palette::green, Palette::blue};
				c.SetEncoderLed(i, blend[0].blend(blend[1], phase));
			}
		}
	}
};
} // namespace Catalyst2::Macro::Ui
