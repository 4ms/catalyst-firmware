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
	}
	void Update(Abstract *&interface) override {
		if (c.button.fine.just_went_high() && p.IsSequenceSelected()) {
			p.CopySequence();
		}
		if (c.button.morph.just_went_high()) {
			p.shared.do_save = true;
		}
		if (!c.button.bank.is_high()) {
			return;
		}
		if (p.shared.modeswitcher.Check()) {
			interface = nullptr;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) override {
		if (c.button.shift.is_high()) {
			// change all channgels.
			auto cm = p.data.settings.GetChannelMode(encoder);
			cm.Inc(dir);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.data.settings.SetChannelMode(i, cm);
				p.shared.quantizer[i].Load(cm.GetScale());
			}
		} else {
			p.data.settings.IncChannelMode(encoder, dir);
			p.shared.quantizer[encoder].Load(p.data.settings.GetChannelMode(encoder).GetScale());
		}
	}
	void OnSceneButtonRelease(uint8_t scene) override {
		if (scene == p.GetSelectedChannel()) {
			p.DeselectSequence();
		} else {
			p.SelectChannel(scene);
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedChannel(), true);

		for (auto i = 0u; i < Model::NumChans; i++) {
			c.SetEncoderLed(i, p.data.settings.GetChannelMode(i).GetColor());
		}
	}
};

} // namespace Catalyst2::Sequencer::Ui