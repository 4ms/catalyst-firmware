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
		c.button.bank.clear_events();
		for (auto &b : c.button.scene) {
			b.clear_events();
		}
	}
	void Update() override {
		if (c.button.bank.just_went_high()) {
			p.SelectChannel();
			SwitchUiMode(main_ui);
			return;
		}

		for (auto button = 0u; button < c.button.scene.size(); button++) {
			if (c.button.scene[button].just_went_low()) {
				p.slot.settings.ToggleMute(button);
			}
		}

		if (c.button.play.just_went_low()) {
			if (c.button.shift.is_high()) {
				p.Stop();
			} else {
				p.TogglePause();
			}
		}

		if (c.button.add.just_went_high()) {
			p.seqclock.Tap();
		}
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		c.SetPlayLed(!p.IsPaused());
		AllChannelStepOutput(outs);
		if (c.button.bank.is_high()) {
			ClearButtonLeds(c);
			c.SetButtonLed(p.GetPrevSelectedChannel(), true);
		} else {
			SetMutedLeds();
		}
	}

	void AllChannelStepOutput(const Model::Output::Buffer &buf) {
		for (auto [chan, val] : countzip(buf)) {
			const auto cm = p.slot.settings.GetChannelMode(chan);
			Color col;
			if (cm.IsMuted()) {
				col = Palette::very_dim_grey;
			} else {
				col = cm.IsGate() ? Palette::Gate::fromOutput(val) : Palette::Cv::fromOutput(val);
			}
			c.SetEncoderLed(chan, col);
		}
	}

	void SetMutedLeds() {
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			const auto is_muted = p.slot.settings.GetChannelMode(chan).IsMuted();
			c.SetButtonLed(chan, !is_muted);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
