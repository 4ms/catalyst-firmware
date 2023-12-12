#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui
{
class Morph : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		if (!p.IsSequenceSelected()) {
			p.SelectChannel(0);
		}
	}
	void Update(Abstract *&interface) override {
		if (!c.button.morph.is_high()) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		p.IncStepModifier(encoder, inc);
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds();
		ClearButtonLeds();

		const auto chan = p.GetSelectedChannel();
		const uint8_t led = p.player.GetPlayheadStepOnPage(chan);
		const auto playheadpage = p.player.GetPlayheadPage(chan);
		const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
		const auto mvals = p.GetPageValuesModifier(page);

		for (auto i = 0u; i < Model::NumChans; i++) {
			if (i == led && page == playheadpage)
				c.SetEncoderLed(led, Palette::seqhead);
			else {
				auto col = Palette::grey.blend(Palette::red, mvals[i]);
				if (mvals[i] == 0.f)
					col = Palette::green;
				c.SetEncoderLed(i, col);
			}
		}
		if (p.IsPageSelected())
			c.SetButtonLed(page, ((p.shared.internalclock.TimeNow() >> 8) & 1) > 0);
		else
			c.SetButtonLed(page, true);
	}
};

} // namespace Catalyst2::Sequencer::Ui
