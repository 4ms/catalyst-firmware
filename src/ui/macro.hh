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
	Add add{p, c, *this};
	Bank bank{p, c, *this};
	Morph morph{p, c, *this};
	Settings settings{p, c, *this};
	Range range{p, c, *this};

public:
	Main(Catalyst2::Macro::Interface &p, Controls &c, Abstract &sequencer)
		: Usual{p, c, sequencer} {
	}
	void Init() override {
		c.button.fine.clear_events();
		c.button.play.clear_events();
		p.shared.modeswitcher.SetAlarm();
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustPressed(c, [this](uint8_t button) { OnSceneButtonPress(button); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.fine.just_went_high() && p.shared.youngest_scene_button.has_value()) {
			p.bank.Copy(p.shared.youngest_scene_button.value());
			ConfirmCopy(p.shared, p.shared.youngest_scene_button.value());
		}

		const auto is_latch = p.mode == Catalyst2::Macro::Mode::Mode::LATCH;

		if (!c.button.fine.is_high() || !c.button.play.is_high() || !c.button.morph.is_high()) {
			p.shared.modeswitcher.SetAlarm();
		}

		if (c.button.play.just_went_high() && !is_latch) {
			p.recorder.Reset();
		}

		if (p.shared.modeswitcher.Check()) {
			p.shared.mode = Model::Mode::Sequencer;
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.shared.blinker.Set(Model::NumChans - i - 1, 1, 200, 100 * i + 250);
			}
			SwitchUiMode(main_ui);
			return;
		} else if (c.button.add.is_high()) {
			if (!p.bank.IsBankClassic() && !is_latch) {
				SwitchUiMode(add);
			}
		} else if (c.button.bank.is_high()) {
			SwitchUiMode(bank);
		} else if (c.button.morph.is_high() && !c.button.fine.is_high()) {
			if (c.button.shift.is_high()) {
				SwitchUiMode(range);
			} else {
				SwitchUiMode(morph);
			}
		} else if (c.button.shift.is_high()) {
			SwitchUiMode(settings);
		}
	}
	void OnSceneButtonPress(uint8_t button) {
		if (p.mode == Catalyst2::Macro::Mode::Mode::LATCH) {
			return;
		}
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
		const auto fine = c.button.fine.is_high() && !c.button.morph.is_high();
		inc = c.button.fine.is_high() && c.button.morph.is_high() ? inc * 12 : inc;

		if (scenebdown) {
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high())
					p.bank.IncChan(i, encoder, inc, fine);
			}
		} else {
			if (p.mode == Catalyst2::Macro::Mode::Mode::LATCH) {
				p.bank.IncChan(p.shared.youngest_scene_button.Last(), encoder, inc, fine);
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
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		c.SetPlayLed(p.recorder.IsPlaying());

		const auto ysb = p.shared.youngest_scene_button;
		if (ysb.has_value()) {
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high())
					c.SetButtonLed(i, true);
			}
		} else if (p.mode == Catalyst2::Macro::Mode::Mode::LATCH) {
			c.SetButtonLed(p.shared.youngest_scene_button.Last(), true);
		} else {
			if (p.recorder.IsRecording()) {
				SceneButtonDisplayRecording();
			} else {
				if (c.button.add.is_high()) {
					c.SetButtonLed(p.bank.pathway.GetSceneB(), true);
				} else {
					const auto l = p.bank.pathway.SceneRelative(-1);
					const auto r = p.bank.pathway.SceneRelative(1);
					if (l == r) {
						c.SetButtonLed(l, true);
					} else {
						const auto pos = p.bank.pathway.GetPhase();
						c.SetButtonLed(l, 1.f - pos);
						c.SetButtonLed(r, pos);
					}
				}
			}
		}

		if (ysb.has_value() && (p.mode == Catalyst2::Macro::Mode::Mode::BLIND)) {
			const auto scene_to_display = ysb.value();
			EncoderDisplayScene(scene_to_display);
		} else {
			for (auto [chan, val] : countzip(outs)) {
				Color col;
				if (p.bank.GetChannelMode(chan).IsGate()) {
					// if channel is a gate, instead of displaying it's actual output, we should display what it is set
					// to.
					const auto scene = p.slew.button.IsRunning() ? std::nullopt :
									   p.mode == Catalyst2::Macro::Mode::Mode::LATCH ?
																   p.shared.youngest_scene_button.Last() :
									   p.shared.youngest_scene_button ? p.shared.youngest_scene_button.value() :
																		p.bank.pathway.CurrentScene();
					col = scene ? Palette::Gate::fromLevelMacro(p.bank.GetGate(scene.value(), chan)) : Palette::off;
				} else {
					col = Palette::Cv::fromOutput(p.shared.data.palette[chan], val);
				}
				c.SetEncoderLed(chan, col);
			}
		}
	}

private:
	void EncoderDisplayScene(Catalyst2::Macro::Pathway::SceneId scene) {
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			const auto col = p.bank.GetChannelMode(chan).IsGate() ?
								 Palette::Gate::fromLevelMacro(p.bank.GetGate(scene, chan)) :
								 Palette::Cv::fromLevel(
									 p.shared.data.palette[chan], p.bank.GetCv(scene, chan), p.bank.GetRange(chan));
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
