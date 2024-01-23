#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui
{
class Bank : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		c.button.morph.clear_events();
		c.button.play.clear_events();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonReleased([this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.fine.just_went_high() && p.IsSequenceSelected()) {
			p.CopySequence();
			ConfirmCopy(p.GetSelectedChannel());
		}
		if (c.button.morph.just_went_high()) {
			p.shared.save.Notify();
		}
		if (p.shared.save.Check() && c.button.morph.is_high()) {
			p.shared.save.Notify();
			p.shared.do_save = true;
		}
		if (!c.button.bank.is_high() && !YoungestSceneButton().has_value()) {
			return;
		}
		if (c.button.shift.is_high()) {
			return;
		}
		if (p.shared.modeswitcher.Check()) {
			interface = nullptr;
			p.shared.data.mode = Model::Mode::Macro;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) {
		p.data.settings.IncChannelMode(encoder, dir);
		p.shared.quantizer[encoder].Load(p.data.settings.GetChannelMode(encoder).GetScale());
	}
	void OnSceneButtonRelease(uint8_t scene) {
		if ((c.button.play.is_high() || c.button.play.just_went_low()) && p.IsSequenceSelected()) {
			p.player.queue.Queue(p.GetSelectedChannel(), scene);
		} else {
			if (scene == p.GetSelectedChannel()) {
				p.DeselectSequence();
			} else {
				p.SelectChannel(scene);
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		if (p.shared.blinker.IsSet()) {
			c.SetButtonLed(p.shared.blinker.Led(), p.shared.blinker.IsHigh());
		} else {
			c.SetButtonLed(p.GetSelectedChannel(), true);
		}
		for (auto i = 0u; i < Model::NumChans; i++) {
			c.SetEncoderLed(i, p.data.settings.GetChannelMode(i).GetColor());
		}
	}
};

} // namespace Catalyst2::Sequencer::Ui
