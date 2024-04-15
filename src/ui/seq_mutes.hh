#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "sequencer.hh"
#include "ui/helper_functions.hh"
#include "util/countzip.hh"
#include <complex>

namespace Catalyst2::Ui::Sequencer
{
class Mutes : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		for (auto &b : c.button.scene) {
			b.clear_events();
		}
	}
	void Update() override {
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.add.just_went_high()) {
			p.seqclock.Tap();
		}
	}
	void OnSceneButtonRelease(uint8_t button) {
		if (c.button.bank.is_high()) {
			p.SelectChannel(button);
			SwitchUiMode(main_ui);
			return;
		}

		p.slot.settings.ToggleMute(button);
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		c.SetPlayLed(!p.seqclock.IsPaused());
		AllChannelStepOutput(outs);
		if (c.button.bank.is_high()) {
			ClearButtonLeds(c);
			c.SetButtonLed(p.GetPrevSelectedChannel(), true);
		}
	}

	void AllChannelStepOutput(const Model::Output::Buffer &buf) {
		for (auto [chan, val] : countzip(buf)) {
			const auto cm = p.slot.settings.GetChannelMode(chan);
			const auto is_muted = cm.IsMuted();
			Color col;
			if (is_muted) {
				col = Palette::very_dim_grey;
			} else {
				col = cm.IsGate() ? Palette::Gate::fromOutput(val) : Palette::Cv::fromOutput(val);
			}
			c.SetButtonLed(chan, !is_muted);
			c.SetEncoderLed(chan, col);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
