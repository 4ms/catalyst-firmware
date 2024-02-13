#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "macro_add.hh"
#include "macro_bank.hh"
#include "macro_common.hh"
#include "macro_morph.hh"
#include "macro_range.hh"
#include "macro_settings.hh"
#include "params.hh"
#include "range.hh"

namespace Catalyst2::Ui::Macro
{

class Main : public Usual {
	Add add{p, c};
	Bank bank{p, c};
	Morph morph{p, c};
	Settings settings{p, c};
	Range range{p, c};
	Abstract &sequencer;

public:
	Main(Catalyst2::Macro::Interface &p, Controls &c, Abstract &sequencer)
		: Usual{p, c}
		, sequencer{sequencer} {
	}
	// using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		c.button.play.clear_events();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustPressed(c, [this](uint8_t button) { OnSceneButtonPress(button); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.fine.just_went_high() && p.shared.youngest_scene_button.has_value()) {
			p.bank.Copy(p.shared.youngest_scene_button.value());
			ConfirmCopy(p.shared, p.shared.youngest_scene_button.value());
		}

		p.main_mode = false;

		if (c.button.play.just_went_high()) {
			p.recorder.Reset();
		}

		if (p.shared.mode == Model::Mode::Sequencer) {
			interface = &sequencer;
			return;
		}

		if (c.button.add.is_high()) {
			p.main_mode = false;
			if (!p.bank.IsBankClassic())
				interface = &add;
			return;
		}
		if (c.button.bank.is_high()) {
			interface = &bank;
			return;
		}

		if (c.button.morph.is_high()) {
			if (c.button.shift.is_high()) {
				interface = &range;
			} else {
				interface = &morph;
			}
			return;
		}

		if (c.button.shift.is_high()) {
			interface = &settings;
			return;
		}

		p.main_mode = true;
		interface = this;
	}
	void OnSceneButtonPress(uint8_t button) {
		if (p.bank.IsBankClassic() && c.button.add.is_high()) {
			p.bank.pathway.ReplaceSceneB(button);
		}
	}
	void OnSceneButtonRelease(uint8_t button) {
		if (c.button.fine.is_high()) {
			p.bank.Paste(button);
			ConfirmPaste(p.shared, button);
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto scenebdown = p.shared.youngest_scene_button.has_value();
		const auto fine = c.button.fine.is_high();

		if (scenebdown) {
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high())
					p.bank.IncChan(i, encoder, inc, fine);
			}
		} else {
			const auto cscene = p.bank.pathway.CurrentScene();
			if (cscene.has_value()) {
				p.bank.IncChan(cscene.value(), encoder, inc, fine);
			} else {
				p.bank.IncChan(p.bank.pathway.SceneRelative(-1), encoder, inc, fine);
				p.bank.IncChan(p.bank.pathway.SceneRelative(1), encoder, inc, fine);
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		c.SetPlayLed(p.recorder.IsPlaying());

		auto ysb = p.shared.youngest_scene_button;
		if (ysb.has_value()) {
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high())
					c.SetButtonLed(i, true);
			}
			const auto scene_to_display = ysb.value();
			EncoderDisplayScene(scene_to_display);
		} else {

			for (auto [chan, val] : countzip(outs)) {
				Color col;
				if (p.bank.GetChannelMode(chan).IsGate()) {
					// if channel is a gate, instead of displaying it's actual output, we should display what it is set
					// to.
					col = p.bank.pathway.OnAScene() ?
							  Palette::Gate::fromLevelMacro(p.bank.GetGate(p.bank.pathway.SceneRelative(), chan)) :
							  Palette::off;
				} else {
					col = Palette::Cv::fromOutput(val);
				}
				c.SetEncoderLed(chan, col);
			}

			if (p.recorder.IsRecording())
				SceneButtonDisplayRecording();
			else {
				if (c.button.add.is_high()) {
					c.SetButtonLed(p.bank.pathway.GetSceneB(), true);
				} else {
					const auto l = p.bank.pathway.SceneRelative(-1);
					const auto r = p.bank.pathway.SceneRelative(1);
					if (l == r)
						c.SetButtonLed(l, true);
					else {
						const auto pos = p.bank.pathway.GetPhase();
						c.SetButtonLed(l, 1.f - pos);
						c.SetButtonLed(r, pos);
					}
				}
			}
		}
	}

private:
	void EncoderDisplayScene(Catalyst2::Macro::Pathway::SceneId scene) {
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			const auto col = p.bank.GetChannelMode(chan).IsGate() ?
								 Palette::Gate::fromLevelMacro(p.bank.GetGate(scene, chan)) :
								 Palette::Cv::fromLevel(p.bank.GetCv(scene, chan), p.bank.GetRange(chan));
			c.SetEncoderLed(chan, col);
		}
	}

	void SceneButtonDisplayRecording() {
		const auto led = static_cast<unsigned>(p.recorder.CapacityFilled() * 8u);
		const auto level = (p.recorder.size() & 0x10) > 0;
		c.SetButtonLed(led, level);
	}
};
} // namespace Catalyst2::Ui::Macro
