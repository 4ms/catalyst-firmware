#pragma once

#include "controls.hh"

#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class Probability : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		if (!p.IsChannelSelected()) {
			p.SelectChannel(0);
		}
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.morph.is_high() && !c.button.shift.is_high()) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.IncStepProbability(encoder, inc);
	}

	void OnSceneButtonRelease(uint8_t button) {
		if (!c.button.fine.just_went_low() && !c.button.shift.just_went_low()) {
			if (p.IsPageSelected() && button == p.GetSelectedPage())
				p.DeselectPage();
			else
				p.SelectPage(button);
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);

		const auto chan = p.GetSelectedChannel();
		const uint8_t led = p.player.GetPlayheadStepOnPage(chan);
		const auto playheadpage = p.player.GetPlayheadPage(chan);
		const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
		const auto mvals = p.GetPageValuesProbability(page);

		for (auto i = 0u; i < Model::NumChans; i++) {
			if (i == led && page == playheadpage) {
				c.SetEncoderLed(led, Palette::SeqHead::color);
			} else {
				auto col = Palette::Probability::color(mvals[i]);
				c.SetEncoderLed(i, col);
			}
		}
		if (p.IsPageSelected()) {
			c.SetButtonLed(page, ((p.shared.internalclock.TimeNow() >> 8) & 1) > 0);
		} else {
			c.SetButtonLed(page, true);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
