#pragma once

#include "conf/palette.hh"
#include "controls.hh"
#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class Probability : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		if (!p.IsChannelSelected()) {
			p.SelectChannel();
		}
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.morph.is_high() && !c.button.shift.is_high()) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		p.IncStepProbability(encoder, inc);
	}

	void OnSceneButtonRelease(uint8_t button) {
		p.SelectPage(button);
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
			auto color = Palette::Probability::color(p.GetStep(step_offset + i).ReadProbability());
			PaintStep(page, i, color);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
