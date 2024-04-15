#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "seq_save.hh"
#include "sequencer.hh"

namespace Catalyst2::Ui::Sequencer
{
class Bank : public Usual {
	Save save{p, c, &main_ui};

public:
	using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		c.button.morph.clear_events();
		c.button.play.clear_events();
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.fine.just_went_high() && p.IsChannelSelected()) {
			p.CopySequence();
			ConfirmCopy(p.shared, p.GetSelectedChannel());
		}
		if (!c.button.bank.is_high() && !p.shared.youngest_scene_button.has_value()) {
			SwitchUiMode(main_ui);
			return;
		}
		if (c.button.shift.is_high()) {
			SwitchUiMode(main_ui);
			return;
		}
		if (c.button.morph.just_went_high()) {
			p.shared.save.SetAlarm();
		}
		if (p.shared.save.Check() && c.button.morph.is_high()) {
			SwitchUiMode(save);
			return;
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) {
		p.slot.settings.IncChannelMode(encoder, dir);
	}
	void OnSceneButtonRelease(uint8_t page) {
		if ((c.button.play.is_high() || c.button.play.just_went_low()) && p.IsChannelSelected()) {
			if (p.seqclock.IsPaused()) {
				p.slot.settings.SetStartOffset(p.GetSelectedChannel(), Catalyst2::Sequencer::SeqPageToStep(page));
				p.player.Reset();
				p.seqclock.Pause();
			} else {
				p.player.queue.Queue(p.GetSelectedChannel(), page);
			}
		} else {
			p.SelectChannel(page);
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		c.SetButtonLed(p.GetSelectedChannel(), true);
		for (auto i = 0u; i < Model::NumChans; i++) {
			c.SetEncoderLed(i, p.slot.settings.GetChannelMode(i).GetColor());
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
