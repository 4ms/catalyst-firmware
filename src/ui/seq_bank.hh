#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "seq_save.hh"

namespace Catalyst2::Ui::Sequencer
{
class Bank : public Usual {
	Save save{p, c};

public:
	using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		c.button.morph.clear_events();
		c.button.play.clear_events();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.fine.just_went_high() && p.IsChannelSelected()) {
			p.CopySequence();
			ConfirmCopy(p.shared, p.GetSelectedChannel());
		}
		if (!c.button.bank.is_high() && !p.shared.youngest_scene_button.has_value()) {
			return;
		}
		if (c.button.shift.is_high()) {
			return;
		}
		const auto time_now = p.shared.internalclock.TimeNow();
		if (c.button.morph.just_went_high()) {
			p.shared.save.SetAlarm(time_now);
		}
		if (p.shared.save.Check(time_now) && c.button.morph.is_high()) {
			interface = &save;
			return;
			// p.shared.do_save = true;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) {
		p.slot.settings.IncChannelMode(encoder, dir);
		p.shared.quantizer[encoder].Load(p.slot.settings.GetChannelMode(encoder).GetScale());
	}
	void OnSceneButtonRelease(uint8_t scene) {
		if ((c.button.play.is_high() || c.button.play.just_went_low()) && p.IsChannelSelected()) {
			p.player.queue.Queue(p.GetSelectedChannel(), scene);
		} else {
			if (scene == p.GetSelectedChannel()) {
				p.DeselectChannel();
			} else {
				p.SelectChannel(scene);
			}
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
