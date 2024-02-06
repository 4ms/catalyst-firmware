#pragma once

#include "controls.hh"
#include "macro_add.hh"
#include "macro_bank.hh"
#include "macro_common.hh"
#include "macro_morph.hh"
#include "macro_range.hh"
#include "macro_settings.hh"
#include "params.hh"

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
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (c.button.fine.just_went_high() && p.shared.youngest_scene_button.has_value()) {
			p.bank.Copy(p.shared.youngest_scene_button.value());
			ConfirmCopy(p.shared, p.shared.youngest_scene_button.value());
		}

		if (p.shared.mode == Model::Mode::Sequencer) {
			interface = &sequencer;
			return;
		}

		if (c.button.add.is_high()) {
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
		interface = this;
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
			if (p.pathway.OnAScene()) {
				p.bank.IncChan(p.pathway.SceneNearest(), encoder, inc, fine);
			} else {
				p.bank.IncChan(p.pathway.SceneLeft(), encoder, inc, fine);
				p.bank.IncChan(p.pathway.SceneRight(), encoder, inc, fine);
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		auto ysb = p.shared.youngest_scene_button;
		if (ysb.has_value()) {
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high())
					c.SetButtonLed(i, true);
			}
			const auto scene_to_display = ysb.value();
			EncoderDisplayScene(scene_to_display);
		} else {
			EncoderDisplayOutput(outs);
			if (p.recorder.IsRecording())
				SceneButtonDisplayRecording();
			else {
				const auto l = p.pathway.SceneLeft();
				const auto r = p.pathway.SceneRight();
				if (l == r)
					c.SetButtonLed(l, true);
				else {
					const auto pos = p.pathway.GetPhase();
					c.SetButtonLed(l, 1.f - pos);
					c.SetButtonLed(r, pos);
				}
			}
		}
	}

private:
	void EncoderDisplayScene(Catalyst2::Macro::Pathway::SceneId scene) {
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			const auto temp = p.bank.GetChannel(scene, chan);
			const auto isgate = p.bank.GetChannelMode(chan).IsGate();
			c.SetEncoderLed(chan, isgate ? Palette::GateBlend(temp.AsGate()) : Palette::CvBlend(temp.AsCV()));
		}
	}

	void SceneButtonDisplayRecording() {
		const auto led = static_cast<unsigned>(p.recorder.CapacityFilled() * 8u);
		const auto level = (p.recorder.size() & 0x10) > 0;
		c.SetButtonLed(led, level);
	}

	void EncoderDisplayOutput(const Model::Output::Buffer &buf) {
		for (auto [chan, val] : countzip(buf)) {
			const auto col = Palette::EncoderBlend(val, p.bank.GetChannelMode(chan).IsGate());
			c.SetEncoderLed(chan, col);
		}
	}
};
} // namespace Catalyst2::Ui::Macro
