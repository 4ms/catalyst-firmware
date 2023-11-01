#pragma once

#include "../params.hh"
#include "abstract.hh"

namespace Catalyst2::Macro::Ui
{

class Usual : public Catalyst2::Ui::Abstract {
public:
	MacroMode::Interface &p;
	Usual(MacroMode::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	virtual void Init() override {
	}
	virtual void Update(Abstract *&interface) override {
	}
	virtual void Common() override {
		if (c.jack.trig.just_went_high()) {
			if (p.recorder.is_cued()) {
				p.shared.clockdivider.Reset();
				p.recorder.record();
			} else {
				p.shared.clockdivider.Update(p.shared.GetClockDiv());
				if (p.shared.clockdivider.Step())
					p.recorder.reset();
			}
		}
		const auto pos = p.recorder.update(c.ReadSlider() + c.ReadCv()) / 4096.f;
		p.shared.SetPos(pos);
		p.pathway.Update(pos);
		p.override_output = c.YoungestSceneButton();
	}
	virtual void OnSceneButtonRelease(uint8_t button) override {
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override {
	}
	virtual void PaintLeds(const Model::OutputBuffer &outs) override {
	}
};

class Add : public Usual {
	bool first;

public:
	using Usual::Usual;
	virtual void Init() override {
		first = true;
		c.button.shift.clear_events();
	}
	virtual void Update(Abstract *&interface) override {
		if (c.button.shift.just_went_high()) {
			if (p.pathway.OnAScene() || p.pathway.SceneLeft() == p.pathway.SceneRight())
				p.pathway.RemoveSceneNearest();
		}
		if (!c.button.add.is_high() && !c.button.shift.is_high())
			return;

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t button) override {
		auto &path = p.pathway;

		if (c.button.shift.is_high()) {
			if (path.OnAScene()) {
				if (path.SceneNearest() == button)
					path.RemoveSceneNearest();
			} else {
				if (path.SceneLeft() == button)
					path.RemoveSceneLeft();
				else if (path.SceneRight() == button)
					path.RemoveSceneRight();
			}
			return;
		}

		if (!first) {
			path.InsertScene(button, true);
			return;
		}

		first = false;

		if (path.OnAScene())
			path.ReplaceScene(button);
		else
			path.InsertScene(button, false);
	}
	virtual void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();

		auto count = p.pathway.size();
		const auto phase = 1.f / (Pathway::MaxPoints / static_cast<float>(count));

		while (count > 8)
			count -= 8;

		c.SetEncoderLedsCount(count, 0, Palette::green.blend(Palette::red, phase));

		if (p.pathway.OnAScene()) {
			c.SetButtonLed(p.pathway.SceneNearest(), true);
		} else {
			c.SetButtonLed(p.pathway.SceneLeft(), true);
			c.SetButtonLed(p.pathway.SceneRight(), true);
		}
	}
};
class Bank : public Usual {
public:
	using Usual::Usual;
	virtual void Update(Abstract *&interface) override {
		if (!c.button.bank.is_high())
			return;

		interface = this;
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		if (c.button.shift.is_high()) {
			// change all channgels.
			auto cm = p.bank.GetChannelMode(encoder);
			cm.Inc(inc);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.bank.SetChanMode(i, cm);
				p.shared.quantizer[i].LoadScale(cm.GetScale());
			}
		} else {
			p.bank.IncChannelMode(encoder, inc);
			p.shared.quantizer[encoder].LoadScale(p.bank.GetChannelMode(encoder).GetScale());
		}
	}

	virtual void OnSceneButtonRelease(uint8_t button) override {
		p.SelectBank(button);
	}

	virtual void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedBank(), true);
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto col = p.bank.GetChannelMode(i).GetColor();
			c.SetEncoderLed(i, col);
		}
	}
};
class Morph : public Usual {
public:
	using Usual::Usual;
	virtual void Update(Abstract *&interface) override {
		if (!c.button.morph.is_high())
			return;

		interface = this;
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		p.IncMorph(encoder, inc);

		if (c.button.shift.is_high()) {
			const auto m = p.GetMorph(encoder);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.SetMorph(i, m);
			}
		}
	}
	virtual void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearEncoderLeds();

		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto morph = p.GetMorph(i);
			auto col = Palette::grey.blend(Palette::red, morph);
			if (morph == 0.f)
				col = Palette::green;

			c.SetEncoderLed(i, col);
		}
	}
};
class Settings : public Usual {
public:
	using Usual::Usual;
	virtual void Init() override {
		p.shared.hang.Cancel();
	}
	virtual void Update(Abstract *&interface) override {
		if (!c.button.shift.is_high())
			return;

		if (c.button.add.is_high())
			return;

		interface = this;
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto time_now = HAL_GetTick();
		const auto hang = p.shared.hang.Check(time_now);
		const auto is_scene = c.YoungestSceneButton().has_value();

		switch (encoder) {
			case Model::EncoderAlts::Random:
				if (is_scene) {
					for (auto [i, b] : countzip(c.button.scene)) {
						if (b.is_high())
							p.bank.IncRandomAmount(i, inc);
					}
				} else {
					if (inc > 0)
						p.shared.randompool.RandomizeScene();
					else
						p.shared.randompool.ClearScene();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::ClockDiv:
				if (is_scene)
					break;
				inc = hang.has_value() ? inc : 0;
				p.shared.IncClockDiv(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();
		c.ClearEncoderLeds();

		const auto is_scene = c.YoungestSceneButton().has_value();
		const auto scene = c.YoungestSceneButton().value_or(0);

		if (is_scene) {
			const auto random = p.bank.GetRandomAmount(scene);
			auto col = Palette::red;
			if (random > 0.f)
				col = Palette::off.blend(Palette::from_raw(p.shared.randompool.GetSeed()), random);
			c.SetEncoderLed(Model::EncoderAlts::Random, col);
		} else {
			const auto time_now = HAL_GetTick();
			const auto hang = p.shared.hang.Check(time_now);
			if (hang.has_value()) {
				if (hang.value() == Model::EncoderAlts::ClockDiv) {
					c.SetEncoderLedsAddition(ClockDivider::GetDivFromIdx(p.shared.GetClockDiv()), Palette::blue);
				}
			} else {
				c.SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::seqhead);
				c.SetEncoderLed(Model::EncoderAlts::Random, Palette::from_raw(p.shared.randompool.GetSeed()));
			}
		}
	}
};

class Main : public Usual {
	Add add{p, c};
	Bank bank{p, c};
	Morph morph{p, c};
	Settings settings{p, c};

public:
	using Usual::Usual;
	virtual void Init() override {
		c.button.fine.clear_events();
		p.shared.internalclock.SetExternal(true);
	}
	virtual void Update(Abstract *&interface) override {
		if (c.button.fine.just_went_high() && p.override_output.has_value())
			p.bank.Copy(p.override_output.value());

		if (c.button.play.just_went_high()) {
			if (c.button.shift.is_high())
				p.recorder.cue_recording();

			if (c.toggle.trig_sense.is_high())
				p.recorder.reset();
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
			interface = &morph;
			return;
		}
		if (c.button.shift.is_high()) {
			interface = &settings;
			return;
		}
		interface = this;
	}
	virtual void OnSceneButtonRelease(uint8_t button) override {
		if (c.button.fine.is_high()) {
			p.bank.Paste(button);
		}
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto scenebdown = c.YoungestSceneButton().has_value();
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
	virtual void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();
		if (c.YoungestSceneButton().has_value()) {
			for (auto [i, b] : countzip(c.button.scene)) {
				if (b.is_high())
					c.SetButtonLed(i, true);
			}
			const auto scene_to_display = c.YoungestSceneButton().value();
			EncoderDisplayScene(scene_to_display);
		} else {
			EncoderDisplayOutput(outs);
			if (p.recorder.is_recordering())
				SceneButtonDisplayRecording();
			else {
				const auto l = p.pathway.SceneLeft();
				const auto r = p.pathway.SceneRight();
				if (l == r)
					c.SetButtonLed(l, true);
				else {
					const auto pos = p.shared.GetPos();
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
			const Color col = Palette::EncoderBlend(temp, p.bank.GetChannelMode(chan).IsGate());
			c.SetEncoderLed(chan, col);
		}
	}

	void SceneButtonDisplayRecording() {
		const auto led = static_cast<unsigned>(p.recorder.capacity_filled() * 8u);
		const auto level = (p.recorder.size() & 0x10) > 0;
		c.SetButtonLed(led, level);
	}

	void EncoderDisplayOutput(const Model::OutputBuffer &buf) {
		for (auto [chan, val] : countzip(buf)) {
			const Color col = Palette::EncoderBlend(val, p.bank.GetChannelMode(chan).IsGate());
			c.SetEncoderLed(chan, col);
		}
	}
};
} // namespace Catalyst2::Macro::Ui