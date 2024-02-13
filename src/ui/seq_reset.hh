#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class Reset : public Usual {
	bool wait = true;

public:
	using Usual::Usual;
	void Init() override {
		wait = true;
		p.Reset(true);
	}
	void Update(Abstract *&interface) override {
		if (wait) {
			if (!c.button.play.is_high() && !c.button.shift.is_high()) {
				wait = false;
			}
		} else {
			if (c.button.add.is_high() || c.button.fine.is_high() || c.button.morph.is_high() ||
				c.button.bank.is_high() || c.button.shift.is_high())
			{
				return;
			}
			for (auto [chan, butt, sequence] : countzip(c.button.scene, p.slot.channel)) {
				if (butt.is_high()) {
					sequence = Catalyst2::Sequencer::ChannelData{};
					p.slot.settings.Clear(chan);
					return;
				}
			}
			if (c.button.play.is_high()) {
				p.slot = Catalyst2::Sequencer::Slot{};
				p.Reset(true);
				return;
			}
		}

		interface = this;
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		c.SetPlayLed(false);
		if ((p.shared.internalclock.TimeNow() >> 10u) & 0x01) {
			SetButtonLedsCount(c, Model::NumChans, true);
			c.SetPlayLed(true);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
