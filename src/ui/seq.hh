#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_bank.hh"
#include "seq_common.hh"
#include "seq_morph.hh"
#include "seq_prob.hh"
#include "seq_settings.hh"
#include "seq_settings_global.hh"
#include <complex>

namespace Catalyst2::Ui::Sequencer
{
class Main : public Usual {
	Bank bank{p, c};
	Morph morph{p, c};
	Probability probability{p, c};
	Settings::Global global_settings{p, c};
	Settings::Channel channel_settings{p, c};
	Abstract &macro;

public:
	Main(Catalyst2::Sequencer::Interface &p, Controls &c, Abstract &macro)
		: Usual{p, c}
		, macro{macro} {
	}
	//	using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		c.button.bank.clear_events();
		c.button.add.clear_events();
		c.button.play.clear_events();
		for (auto &b : c.button.scene) {
			b.clear_events();
		}
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonPressed(c, [this](uint8_t button) { OnSceneButtonPress(button); });
		ForEachSceneButtonReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		const auto ysb = p.shared.youngest_scene_button;
		if (c.button.play.just_went_high()) {
			if (ysb.has_value()) {
				if (!p.seqclock.IsPaused()) {
					p.player.songmode.Cancel();
					if (p.player.songmode.IsQueued()) {
						p.data.settings.SetStartOffset(ysb.value() * Model::SeqStepsPerPage);
					} else {
						p.player.queue.Queue(ysb.value());
					}
				} else {
					p.data.settings.SetStartOffset(ysb.value() * Model::SeqStepsPerPage);
					p.player.Reset();
					p.seqclock.Pause();
				}
			} else {
				p.seqclock.Pause();
			}
		}

		c.SetPlayLed(!p.seqclock.IsPaused());

		if (c.button.add.just_went_high()) {
			p.seqclock.Tap(p.shared.internalclock.TimeNow());
		}
		if (p.IsChannelSelected()) {
			if (c.button.fine.just_went_high() && ysb.has_value()) {
				p.shared.did_copy = true;
				p.CopyPage(ysb.value());
				ConfirmCopy(p.shared, ysb.value());
			}
			if (c.button.bank.just_went_high() && c.button.fine.is_high()) {
				p.PasteSequence();
				ConfirmPaste(p.shared, p.GetSelectedChannel());
			}
		}

		if (p.shared.mode == Model::Mode::Macro) {
			interface = &macro;
			return;
		}

		const auto bmorph = c.button.morph.is_high();
		const auto bbank = c.button.bank.is_high();
		if (c.button.shift.is_high()) {
			if (bbank) {
				interface = &channel_settings;
			} else if (bmorph) {
				interface = &probability;
			} else {
				interface = &global_settings;
			}
			return;
		}

		if (bmorph) {
			interface = &morph;
			return;
		}
		if (bbank) {
			interface = &bank;
			return;
		}

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		if (!p.IsChannelSelected()) {
			return;
		}
		const auto fine = c.button.fine.is_high();
		p.IncStep(encoder, inc, fine);
	}
	void OnSceneButtonPress(uint8_t button) {
		if (!p.IsChannelSelected()) {
			p.SelectChannel(button);
		}
		if (c.button.fine.is_high()) {
			p.PastePage(button);
			Catalyst2::Ui::ConfirmPaste(p.shared, button);
			p.shared.did_paste = true;
		}
	}
	void OnSceneButtonRelease(uint8_t button) {
		if (p.shared.did_paste) {
			p.shared.did_paste = false;
			return;
		}
		if (p.shared.did_copy) {
			p.shared.did_copy = false;
			return;
		}
		if (c.button.fine.is_high()) {
			return;
		}
		if (c.button.play.just_went_low() || c.button.play.is_high()) {
			return;
		}
		if (p.IsPageSelected() && button == p.GetSelectedPage()) {
			p.DeselectPage();
		} else {
			p.SelectPage(button);
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);

		if (p.IsChannelSelected()) {
			const auto chan = p.GetSelectedChannel();
			const auto is_gate = p.data.settings.GetChannelMode(chan).IsGate();
			const auto playheadpage = p.player.GetPlayheadPage(chan);
			const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
			if (is_gate && c.button.fine.is_high()) {
				// display gate timing offset
				const auto led = p.player.GetPlayheadStepOnPage(chan);
				const auto pvals = p.GetPageValuesTrigDelay(page);

				for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
					if (i == led && page == playheadpage) {
						c.SetEncoderLed(led, Palette::SeqHead::color);
					} else {
						c.SetEncoderLed(i, Palette::TrigDelayBlend(pvals[i]));
					}
				}

			} else {
				const auto led = p.player.GetPlayheadStepOnPage(chan);
				const auto pvals = is_gate ? p.GetPageValuesGate(page) : p.GetPageValuesCv(page);
				auto display_func = is_gate ? [](Model::Output::type v) { return Palette::GateBlend(v); } :
											  [](Model::Output::type v) { return Palette::CvBlend(v); };

				for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
					if (i == led && page == playheadpage) {
						c.SetEncoderLed(led, Palette::SeqHead::color);
					} else {
						c.SetEncoderLed(i, display_func(pvals[i]));
					}
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

} // namespace Catalyst2::Ui::Sequencer
