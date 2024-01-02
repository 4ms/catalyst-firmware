#pragma once

#include "controls.hh"
#include "macro_add.hh"
#include "macro_bank.hh"
#include "macro_common.hh"
#include "macro_morph.hh"
#include "macro_range.hh"
#include "macro_reset.hh"
#include "macro_settings.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Main : public Usual {
	Add add{p, c};
	Bank bank{p, c};
	Morph morph{p, c};
	Settings settings{p, c};
	Range range{p, c};
	Reset reset{p, c};

public:
	using Usual::Usual;
	void Init() override {
		c.button.fine.clear_events();
		p.shared.internalclock.SetExternal(true);
	}
	void Update(Abstract *&interface) override {
		if (c.button.fine.just_went_high() && p.override_output.has_value())
			p.bank.Copy(p.override_output.value());

		if (p.shared.reset.Check()) {
			p.shared.reset.Notify(false);
			interface = &reset;
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
		if (c.button.morph.is_high() && c.button.shift.is_high()) {
			interface = &range;
			return;
		}
		if (c.button.morph.is_high()) {
			interface = &morph;
			return;
		}
		if (c.button.shift.is_high()) {
			interface = &settings;
			return;
		}
		interface = this;
	}
	void OnSceneButtonRelease(uint8_t button) override {
		if (c.button.fine.is_high()) {
			p.bank.Paste(button);
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto scenebdown = YoungestSceneButton().has_value();
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
		ClearButtonLeds();
		auto ysb = YoungestSceneButton();
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
	void EncoderDisplayScene(Pathway::SceneId scene) {
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
} // namespace Catalyst2::Macro::Ui
