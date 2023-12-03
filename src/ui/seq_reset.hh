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
		p.player.Stop();
	}
	void Update(Abstract *&interface) {
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
			for (auto [inc, b, chan] : countzip(c.button.scene, p.data.channel)) {
				if (b.is_high()) {
					chan = Sequencer::ChannelData{};
					p.shared.save.Update();
					return;
				}
			}
			if (c.button.play.is_high()) {
				p.data = SeqMode::Data{};
				p.player.Stop();
				p.shared.save.Update();
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