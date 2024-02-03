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
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		if (!c.button.morph.is_high() && !c.button.shift.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t channel, int32_t inc) {
		inc = p.shared.hang.Check(p.shared.internalclock.TimeNow()).has_value() ? inc : 0;
		p.bank.IncRange(channel, inc);
		p.shared.hang.Set(channel, p.shared.internalclock.TimeNow());
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();
		const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());
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
