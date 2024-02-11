#pragma once

#include "controls.hh"

#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class Morph : public Usual {
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

		if (!c.button.morph.is_high()) {
			return;
		}
		if (c.button.shift.is_high()) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.IncStepModifier(encoder, inc);
	}

	void OnSceneButtonRelease(uint8_t button) {
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
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);

		const auto playheadpage = p.player.GetPlayheadPage(p.GetSelectedChannel());
		uint8_t page;
		if (p.IsPageSelected()) {
			page = p.GetSelectedPage();
			BlinkSelectedPage(page);
		} else {
			page = playheadpage;
			c.SetButtonLed(playheadpage, true);
		}
		const uint8_t step_offset = Catalyst2::Sequencer::SeqPageToStep(page);

		for (auto i = 0u; i < Model::NumChans; i++) {
			c.SetEncoderLed(i, Palette::Morph::color(p.GetStep(step_offset + i).ReadMorph()));
		}
		if (page == playheadpage) {
			SetPlayheadLed();
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
