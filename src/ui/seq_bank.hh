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
	}
	void Update(Abstract *&interface) override {
		if (c.button.fine.just_went_high() && p.IsSequenceSelected())
			p.seq.CopySequence(p.GetSelectedSequence());

		if (!c.button.bank.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) override {
		if (c.button.shift.is_high()) {
			// change all channgels.
			auto &cm = p.seq.Channel(encoder).mode;
			cm.Inc(dir);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.seq.Channel(i).mode = cm;
				p.shared.quantizer[i].Load(cm.GetScale());
			}
		} else {
			p.seq.Channel(encoder).mode.Inc(dir);
			p.shared.quantizer[encoder].Load(p.seq.Channel(encoder).mode.GetScale());
		}
	}
	void OnSceneButtonRelease(uint8_t scene) override {
		if (scene == p.GetSelectedSequence())
			p.DeselectSequence();
		else
			p.SelectSequence(scene);
	}
	void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedSequence(), true);

		for (auto i = 0u; i < Model::NumChans; i++)
			c.SetEncoderLed(i, p.seq.Channel(i).mode.GetColor());
	}
};

} // namespace Catalyst2::Sequencer::Ui