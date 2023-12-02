#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_bank.hh"
#include "seq_common.hh"
#include "seq_morph.hh"
#include "seq_reset.hh"
#include "seq_settings.hh"

namespace Catalyst2::Sequencer::Ui
{
class Main : public Usual {
	Bank bank{p, c};
	Morph morph{p, c};
	Settings settings{p, c};
	Reset reset{p, c};

public:
	using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		c.button.bank.clear_events();
	}
	void Update(Abstract *&interface) override {
		if (p.shared.reset.Check()) {
			p.shared.reset.Notify(false);
			interface = &reset;
			return;
		}
		if (p.IsSequenceSelected()) {
			auto ysb = YoungestSceneButton();
			if (c.button.fine.just_went_high() && ysb.has_value()) {
				p.CopyPage(ysb.value());
			}
			if (c.button.bank.just_went_high() && c.button.fine.is_high()) {
				p.PasteSequence();
			}
		}
		if (c.button.shift.is_high()) {
			interface = &settings;
			return;
		}
		if (c.button.bank.is_high()) {
			interface = &bank;
			return;
		}
		if (c.button.morph.is_high()) {
			interface = &morph;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		if (!p.IsSequenceSelected())
			return;

		const auto fine = c.button.fine.is_high();
		p.IncStep(encoder, inc, fine);
	}
	void OnSceneButtonRelease(uint8_t button) override {
		if (!p.IsSequenceSelected()) {
			p.SelectChannel(button);
		} else {
			if (c.button.fine.is_high()) {
				p.PastePage(button);
			} else {
				if (!c.button.fine.just_went_low() && !c.button.shift.just_went_low()) {
					if (p.IsPageSelected() && button == p.GetSelectedPage())
						p.DeselectPage();
					else
						p.SelectPage(button);
				}
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();

		if (p.IsSequenceSelected()) {
			const auto chan = p.GetSelectedChannel();
			const uint8_t led = p.player.GetPlayheadStepOnPage(chan, p.shared.GetPos());
			const auto playheadpage = p.player.GetPlayheadPage(chan, p.shared.GetPos());
			const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
			const auto pvals = p.GetPageValues(page);

			for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
				if (i == led && page == playheadpage) {
					c.SetEncoderLed(led, Palette::seqhead);
				} else {
					c.SetEncoderLed(i, Palette::EncoderBlend(pvals[i], p.data.settings.GetChannelMode(chan).IsGate()));
				}
			}
			if (p.IsPageSelected()) {
				c.SetButtonLed(page, ((p.shared.internalclock.TimeNow() >> 8) & 1) > 0);
			} else {
				c.SetButtonLed(page, true);
			}
		} else {
			EncoderDisplayOutput(outs);
		}
	}

	void EncoderDisplayOutput(const Model::Output::Buffer &buf) {
		for (auto [chan, val] : countzip(buf)) {
			Color col = Palette::EncoderBlend(val, p.data.settings.GetChannelMode(chan).IsGate());
			c.SetEncoderLed(chan, col);
		}
	}
};

} // namespace Catalyst2::Sequencer::Ui