#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_bank.hh"
#include "seq_common.hh"
#include "seq_morph.hh"
#include "seq_page_params.hh"
#include "seq_prob.hh"
#include "seq_settings.hh"
#include "seq_settings_global.hh"
#include "sequencer.hh"
#include "sequencer_step.hh"
#include "util/countzip.hh"
#include <complex>

namespace Catalyst2::Ui::Sequencer
{
class Main : public Usual {
	Bank bank{p, c, this};
	Morph morph{p, c, this};
	Probability probability{p, c, this};
	Settings::Global global_settings{p, c, this};
	Settings::Channel channel_settings{p, c, this};
	PageParams page_params{p, c, this};
	Abstract &macro;
	bool just_queued = false;

public:
	Main(Catalyst2::Sequencer::Interface &p, Controls &c, Abstract &macro)
		: Usual{p, c, this}
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
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustPressed(c, [this](uint8_t button) { OnSceneButtonPress(button); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		const auto ysb = p.shared.youngest_scene_button;

		if (c.button.play.just_went_high()) {
			if (ysb.has_value()) {
				if (!p.IsPaused()) {
					p.player.songmode.Cancel();
					if (p.player.songmode.IsQueued()) {
						p.slot.settings.SetStartOffset(ysb.value() * Model::Sequencer::Steps::PerPage);
					} else {
						p.player.queue.Queue(ysb.value());
					}
				} else {
					p.slot.settings.SetStartOffset(ysb.value() * Model::Sequencer::Steps::PerPage);
					p.player.Reset();
					p.TogglePause();
				}
				just_queued = true;
			} else {
				p.TogglePause();
			}
		}
		if (!ysb.has_value() && c.button.play.just_went_low()) {
			just_queued = false;
		}

		if (c.button.add.just_went_high()) {
			p.seqclock.Tap();
		}
		if (c.button.fine.just_went_high() && ysb.has_value()) {
			p.shared.did_copy = true;
			p.CopyPage(ysb.value());
			ConfirmCopy(p.shared, ysb.value());
		}
		if (c.button.bank.just_went_high() && c.button.fine.is_high()) {
			p.PasteSequence();
			ConfirmPaste(p.shared, p.GetSelectedChannel());
		}

		const auto bmorph = c.button.morph.is_high();
		const auto bbank = c.button.bank.is_high();

		if (p.shared.mode == Model::Mode::Macro) {
			SwitchUiMode(macro);
		} else if (c.button.shift.is_high()) {
			if (bbank) {
				SwitchUiMode(channel_settings);
			} else if (bmorph) {
				SwitchUiMode(probability);
			} else if (ysb) {
				SwitchUiMode(page_params);
			} else {
				SwitchUiMode(global_settings);
			}
		} else if (bmorph && !c.button.fine.is_high()) {
			SwitchUiMode(morph);
		} else if (bbank) {
			SwitchUiMode(bank);
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto fine = c.button.fine.is_high() && !c.button.morph.is_high();
		inc = c.button.fine.is_high() && c.button.morph.is_high() ? inc * 12 : inc;
		p.IncStep(encoder, inc, fine);
	}
	void OnSceneButtonPress(uint8_t button) {
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
		if (c.button.fine.is_high() || c.button.play.is_high()) {
			return;
		}
		if (just_queued) {
			just_queued = false;
			return;
		}
		p.SelectPage(button);
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		c.SetPlayLed(!p.IsPaused());
		ClearButtonLeds(c);
		const auto playheadpage = p.GetPlayheadPage();

		uint8_t page;
		if (p.IsPageSelected()) {
			page = p.GetSelectedPage();
			BlinkSelectedPage(page);
		} else {
			page = playheadpage;
			c.SetButtonLed(playheadpage, true);
		}

		if constexpr (BuildOptions::ManualColorMode) {
			ManualColorTestMode(page);
			return;
		}

		PaintStepValues(page);
	}

	void ManualColorTestMode(uint8_t page) {
		const auto page_start = Catalyst2::Sequencer::SeqPageToStep(page);
		auto red = p.GetStep(page_start).ReadCv();
		auto green = p.GetStep(page_start + 1).ReadCv();
		auto blue = p.GetStep(page_start + 2).ReadCv();
		Color col = Palette::ManualRGB(red, green, blue);
		c.SetEncoderLed(0, Palette::red);
		c.SetEncoderLed(1, Palette::green);
		c.SetEncoderLed(2, Palette::blue);
		c.SetEncoderLed(3, col);
		c.SetEncoderLed(4, col);
		c.SetEncoderLed(5, col);
		c.SetEncoderLed(6, col);
		c.SetEncoderLed(7, col);
	}
};

} // namespace Catalyst2::Ui::Sequencer
