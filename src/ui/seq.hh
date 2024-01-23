#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_bank.hh"
#include "seq_common.hh"
#include "seq_morph.hh"
#include "seq_reset.hh"
#include "seq_settings.hh"
#include "ui/seq_settings_global.hh"
#include "ui/seq_song_mode.hh"

namespace Catalyst2::Sequencer::Ui
{
class Main : public Usual {
	Bank bank{p, c};
	Morph morph{p, c};
	Settings::Global global_settings{p, c};
	Settings::Channel channel_settings{p, c};
	Reset reset{p, c};
	Ui::SongMode songmode{p, c};

public:
	using Usual::Usual;
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
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonPressed([this](uint8_t button) { OnSceneButtonPress(button); });
		ForEachSceneButtonReleased([this](uint8_t button) { OnSceneButtonRelease(button); });

		const auto ysb = YoungestSceneButton();
		if (c.button.play.just_went_high()) {
			if (ysb.has_value()) {
				if (!p.player.IsPaused()) {
					p.player.songmode.Cancel();
					p.player.queue.Queue(ysb.value());
				} else {
					p.data.settings.SetStartOffset(ysb.value() * Model::SeqStepsPerPage);
					p.player.Reset();
					p.player.TogglePause();
				}
			} else {
				p.player.TogglePause();
			}
		}

		if (!ysb.has_value()) {
			if (c.button.play.just_went_low()) {
				p.shared.reset.Notify(false);
			}
		}
		c.SetPlayLed(!p.player.IsPaused());

		if (c.button.add.just_went_high()) {
			p.shared.internalclock.Tap();
		}
		if (p.IsSequenceSelected()) {
			if (c.button.fine.just_went_high() && ysb.has_value()) {
				p.shared.did_copy = true;
				p.CopyPage(ysb.value());
				ConfirmCopy(ysb.value());
			}
			if (c.button.bank.just_went_high() && c.button.fine.is_high()) {
				p.PasteSequence();
				ConfirmPaste(p.GetSelectedChannel());
			}
		}
		if (p.shared.reset.Check()) {
			p.shared.reset.Notify(false);
			interface = &reset;
			return;
		}
		const auto bshift = c.button.shift.is_high();
		const auto bbank = c.button.bank.is_high();
		if (bshift && ysb.has_value() && c.button.play.is_high()) {
			interface = &songmode;
			return;
		}
		if (bshift && bbank) {
			interface = &channel_settings;
			return;
		}
		if (bshift) {
			interface = &global_settings;
			return;
		}
		if (bbank) {
			interface = &bank;
			return;
		}
		if (c.button.morph.is_high()) {
			interface = &morph;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		if (!p.IsSequenceSelected()) {
			return;
		}
		const auto fine = c.button.fine.is_high();
		p.IncStep(encoder, inc, fine);
	}
	void OnSceneButtonPress(uint8_t button) {
		if (!p.IsSequenceSelected()) {
			p.SelectChannel(button);
		}
		if (c.button.fine.is_high()) {
			p.PastePage(button);
			ConfirmPaste(button);
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
		if (c.button.play.just_went_low()) {
			return;
		}
		if (c.button.play.is_high() || c.button.play.just_went_low()) {
			return;
		}
		if (p.IsPageSelected() && button == p.GetSelectedPage()) {
			p.DeselectPage();
		} else {
			p.SelectPage(button);
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();

		if (p.IsSequenceSelected()) {
			const auto chan = p.GetSelectedChannel();
			const auto led = p.player.GetPlayheadStepOnPage(chan);
			const auto playheadpage = p.player.GetPlayheadPage(chan);
			const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
			const auto pvals = p.GetPageValues(page);
			const auto is_gate = p.data.settings.GetChannelMode(chan).IsGate();
			const auto offset = Model::SeqStepsPerPage * page;

			for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
				if (i == led && page == playheadpage && static_cast<int8_t>(i + offset) != p.GetHiddenStep()) {
					c.SetEncoderLed(led, Palette::SeqHead::color);
				} else {
					c.SetEncoderLed(i, Palette::EncoderBlend(pvals[i], is_gate));
				}
			}
			const auto &b = p.shared.blinker;
			if (b.IsSet()) {
				c.SetButtonLed(b.Led(), b.IsHigh());
			} else {
				if (p.IsPageSelected()) {
					c.SetButtonLed(page, ((p.shared.internalclock.TimeNow() >> 8) & 1) > 0);
				} else {
					c.SetButtonLed(page, true);
				}
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
