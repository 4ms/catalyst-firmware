#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui
{
class Reset : public Usual {
	bool wait = true;

public:
	using Usual::Usual;
	void Init() override {
		wait = true;
		p.shared.internalclock.pause = true;
		p.player.Reset();
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
			for (auto [chan, butt, sequence] : countzip(c.button.scene, p.data.channel)) {
				if (butt.is_high()) {
					sequence = Sequencer::ChannelData{};
					p.data.settings.Clear(chan);
					return;
				}
			}
			if (c.button.play.is_high()) {
				p.data = Sequencer::Data{};
				p.shared.internalclock.pause = true;
				p.player.Reset();
				return;
			}
		}

		interface = this;
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds();
		ClearButtonLeds();
		if (wait) {
			return;
		}
		if ((p.shared.internalclock.TimeNow() >> 10u) & 0x01) {
			SetButtonLedsCount(Model::NumScenes, true);
		}
	}
};

} // namespace Catalyst2::Sequencer::Ui
