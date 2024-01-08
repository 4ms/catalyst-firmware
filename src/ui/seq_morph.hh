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
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonReleased([this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.morph.is_high()) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.IncStepModifier(encoder, inc);
	}

	void OnSceneButtonRelease(uint8_t button) {
		if (!p.IsSequenceSelected()) {
			p.SelectChannel(button);
		} else {
			if (c.button.fine.is_high()) {
				p.PastePage(button);
			} else {
				if (!c.button.fine.just_went_low() && !c.button.shift.just_went_low()) {
					if (p.IsPageSelected() && button == p.GetSelectedPage())
						p.DeselectPage();
					else
						p.SelectPage(button);
				}
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds();
		ClearButtonLeds();

		const auto chan = p.GetSelectedChannel();
		const uint8_t led = p.player.GetPlayheadStepOnPage(chan);
		const auto playheadpage = p.player.GetPlayheadPage(chan);
		const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
		const auto mvals = p.GetPageValuesModifier(page);
		const auto offset = Model::SeqStepsPerPage * page;

		for (auto i = 0u; i < Model::NumChans; i++) {
			if (i == led && page == playheadpage && static_cast<int8_t>(i + offset) != p.GetHiddenStep()) {
				c.SetEncoderLed(led, Palette::SeqHead::color);
			} else {
				auto col = Palette::Morph::color(1.f - mvals[i]);
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

} // namespace Catalyst2::Sequencer::Ui
