#pragma once
#include "clockdivider.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "intclock.hh"
#include "outputs.hh"
#include "params.hh"
#include "randompool.hh"
#include "recorder.hh"
#include "util/countzip.hh"

#include <functional>

namespace Catalyst2::Ui
{

Color EncoderBlend(uint16_t level, bool chan_type_gate);

class Abstract {
public:
	Controls &c;
	Abstract(Controls &c)
		: c{c}
	{}

	virtual void Init() = 0;
	virtual bool Update() = 0;
	virtual void OnSceneButtonRelease(uint8_t button) = 0;
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) = 0;
	virtual void PaintLeds(const Model::OutputBuffer &outs) = 0;
};

class Macro : public Abstract {
public:
	MacroMode::Interface &p;
	Macro(MacroMode::Interface &p, Controls &c)
		: Abstract{c}
		, p{p}
	{}
	virtual void Init() override
	{
		c.button.fine.clear_events();
	}
	virtual bool Update() override
	{
		Common();

		if (c.button.fine.just_went_high() && p.shared.override_output.has_value())
			p.bank.Copy(p.shared.override_output.value());

		if (c.button.play.just_went_high()) {
			if (c.button.shift.is_high())
				p.recorder.cue_recording();

			if (c.toggle.trig_sense.is_high())
				p.recorder.reset();
		}

		return RequestModeUpdate();
	}

	virtual void OnSceneButtonRelease(uint8_t button) override
	{
		if (c.button.fine.is_high()) {
			p.bank.Paste(button);
		}
	}

	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
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
	virtual void PaintLeds(const Model::OutputBuffer &outs) override
	{
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
	void Common()
	{
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
		p.shared.override_output = c.YoungestSceneButton();
	}

private:
	bool RequestModeUpdate()
	{
		if (c.toggle.mode.is_high())
			return true;

		if (c.button.add.is_high())
			return true;

		if (c.button.bank.is_high())
			return true;

		if (c.button.morph.is_high())
			return true;

		if (c.button.shift.is_high())
			return true;

		return false;
	}

	void EncoderDisplayScene(Pathway::SceneId scene)
	{
		for (auto chan = 0u; chan < Model::NumChans; chan++) {
			const auto temp = p.bank.GetChannel(scene, chan);
			const Color col = EncoderBlend(temp, p.bank.GetChannelMode(chan).IsGate());
			c.SetEncoderLed(chan, col);
		}
	}

	void SceneButtonDisplayRecording()
	{
		const auto led = static_cast<unsigned>(p.recorder.capacity_filled() * 8u);
		const auto level = (p.recorder.size() & 0x10) > 0;
		c.SetButtonLed(led, level);
	}

	void EncoderDisplayOutput(const Model::OutputBuffer &buf)
	{
		for (auto [chan, val] : countzip(buf)) {
			const Color col = EncoderBlend(val, p.bank.GetChannelMode(chan).IsGate());
			c.SetEncoderLed(chan, col);
		}
	}
};

class MacroAdd : public Macro {
	bool first;

public:
	using Macro::Macro;

	virtual void Init() override
	{
		first = true;
		c.button.shift.clear_events();
	}
	virtual bool Update() override
	{
		Common();

		if (c.button.shift.just_went_high()) {
			if (p.pathway.OnAScene() || p.pathway.SceneLeft() == p.pathway.SceneRight())
				p.pathway.RemoveSceneNearest();
		}

		return RequestModeUpdate();
	}

	void OnSceneButtonRelease(uint8_t button) override
	{
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

	virtual void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{}

	virtual void PaintLeds(const Model::OutputBuffer &outs) override
	{
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

private:
	bool RequestModeUpdate()
	{
		if (!c.button.add.is_high() && !c.button.shift.is_high())
			return true;

		return false;
	}
};

class MacroBank : public Macro {
public:
	using Macro::Macro;

	virtual bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
		if (c.button.shift.is_high()) {
			// change all channgels.
			auto cm = p.bank.GetChannelMode(encoder);
			for (auto i = 0u; i < Model::NumChans; i++) {
				cm.Inc(inc);
				p.bank.SetChanMode(i, cm);
				p.shared.quantizer[i].load_scale(cm.GetScale());
			}
		} else {
			p.bank.IncChannelMode(encoder, inc);
			p.shared.quantizer[encoder].load_scale(p.bank.GetChannelMode(encoder).GetScale());
		}
	}

	virtual void OnSceneButtonRelease(uint8_t button) override
	{
		p.SelectBank(button);
	}

	virtual void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedBank(), true);
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto col = p.bank.GetChannelMode(i).GetColor();
			c.SetEncoderLed(i, col);
		}
	}

private:
	bool RequestModeUpdate()
	{
		if (!c.button.bank.is_high())
			return true;

		if (c.button.shift.is_high())
			return true;

		return false;
	}
};

class MacroMorph : public Macro {
public:
	using Macro::Macro;

	virtual bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
		if (encoder != 6)
			return;

		p.IncMorph(inc);
	}

	virtual void OnSceneButtonRelease(uint8_t button) override
	{}

	virtual void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearEncoderLeds();

		const auto morph = p.GetMorph();
		auto col = Palette::grey.blend(Palette::red, morph);
		if (morph == 0.f)
			col = Palette::green;

		c.SetEncoderLed(6, col);
	}

private:
	bool RequestModeUpdate()
	{
		if (!c.button.morph.is_high())
			return true;

		return false;
	}
};

class Seq : public Abstract {
public:
	SeqMode::Interface &p;
	Seq(SeqMode::Interface &p, Controls &c)
		: Abstract{c}
		, p{p}
	{}
	virtual void Init() override
	{
		return;
		// scratch pad
		// p.IncLength()
	}
	virtual bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
		if (!p.IsSequenceSelected())
			return;

		const auto fine = c.button.fine.is_high();
		p.IncStep(encoder, inc, fine);
	}
	virtual void OnSceneButtonRelease(uint8_t button) override
	{
		if (!p.IsSequenceSelected()) {
			p.SelectSequence(button);
		} else {
			if (p.IsPageSelected() && button == p.GetSelectedPage())
				p.DeselectPage();
			else
				p.SelectPage(button);
		}
	}
	virtual void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearButtonLeds();

		if (p.IsSequenceSelected()) {
			const auto chan = p.GetSelectedSequence();
			const uint8_t led = p.seq.GetPlayheadStepOnPage(chan);
			const auto playheadpage = p.seq.GetPlayheadPage(chan);
			const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
			const auto pvals = p.seq.GetPageValues(chan, page);

			for (auto i = 0u; i < Model::NumChans; i++) {
				if (i == led && page == playheadpage)
					c.SetEncoderLed(led, Palette::seqhead);
				else
					c.SetEncoderLed(i, EncoderBlend(pvals[i], p.seq.Channel(chan).mode.IsGate()));
			}
			if (p.IsPageSelected())
				c.SetButtonLed(page, ((HAL_GetTick() >> 8) & 1) > 0);
			else
				c.SetButtonLed(page, true);

		} else {
			EncoderDisplayOutput(outs);
		}
	}

	void Common()
	{
		if (c.jack.trig.just_went_high()) {
			p.shared.clockdivider.Update(p.shared.GetClockDiv());
		}

		if (c.jack.reset.is_high()) {
			if (c.toggle.trig_sense.is_high())
				p.internalclock.Reset();
			else
				p.shared.clockdivider.Reset();

			p.seq.Reset();
			return;
		}

		bool step;

		if (c.toggle.trig_sense.is_high()) {
			p.internalclock.update();
			step = p.internalclock.step();
		} else {
			step = p.shared.clockdivider.Step();
		}

		if (step)
			p.seq.Step();
	}

	void EncoderDisplayOutput(const Model::OutputBuffer &buf)
	{
		for (auto [chan, val] : countzip(buf)) {
			Color col = EncoderBlend(val, p.seq.Channel(chan).mode.IsGate());
			c.SetEncoderLed(chan, col);
		}
	}

private:
	bool RequestModeUpdate()
	{
		if (!c.toggle.mode.is_high())
			return true;

		if (c.button.shift.is_high() && p.IsSequenceSelected())
			return true;

		if (c.button.bank.is_high())
			return true;

		if (c.button.morph.is_high())
			return true;

		return false;
	}
};

class SeqBank : public Seq {
public:
	using Seq::Seq;
	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		if (c.button.shift.is_high()) {
			// change all channgels.
			auto &cm = p.seq.Channel(encoder).mode;
			cm.Inc(dir);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.seq.Channel(i).mode = cm;
				p.shared.quantizer[i].load_scale(cm.GetScale());
			}
		} else {
			p.seq.Channel(encoder).mode.Inc(dir);
			p.shared.quantizer[encoder].load_scale(p.seq.Channel(encoder).mode.GetScale());
		}
	}

	void OnSceneButtonRelease(uint8_t scene) override
	{
		p.DeselectPage();

		if (scene == p.GetSelectedSequence())
			p.DeselectSequence();
		else
			p.SelectSequence(scene);
	}

	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedSequence(), true);

		for (auto i = 0u; i < Model::NumChans; i++)
			c.SetEncoderLed(i, p.seq.Channel(i).mode.GetColor());
	}

private:
	bool RequestModeUpdate()
	{
		if (!c.button.bank.is_high())
			return true;

		if (c.button.shift.is_high())
			return true;

		return false;
	}
};

class SeqMorph : public Seq {
public:
	using Seq::Seq;

	virtual void Init() override
	{
		if (!p.IsSequenceSelected())
			p.SelectSequence(0);
	}

	virtual bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
		p.IncStepMorph(encoder, inc);
	}

	virtual void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearEncoderLeds();
		c.ClearButtonLeds();

		const auto chan = p.GetSelectedSequence();
		const uint8_t led = p.seq.GetPlayheadStepOnPage(chan);
		const auto playheadpage = p.seq.GetPlayheadPage(chan);
		const auto page = p.IsPageSelected() ? p.GetSelectedPage() : playheadpage;
		const auto mvals = p.seq.GetPageValuesMorph(chan, page);

		for (auto i = 0u; i < Model::NumChans; i++) {
			if (i == led && page == playheadpage)
				c.SetEncoderLed(led, Palette::seqhead);
			else {
				auto col = Palette::grey.blend(Palette::red, mvals[i]);
				if (mvals[i] == 0.f)
					col = Palette::green;
				c.SetEncoderLed(i, col);
			}
		}
		if (p.IsPageSelected())
			c.SetButtonLed(page, ((HAL_GetTick() >> 8) & 1) > 0);
		else
			c.SetButtonLed(page, true);
	}

private:
	bool RequestModeUpdate()
	{
		if (!c.button.morph.is_high())
			return true;

		return false;
	}
};

class SeqSettings : public Seq {

public:
	using Seq::Seq;
	void Init() override
	{
		p.shared.hang.Cancel();
	}
	virtual bool Update() override
	{
		Common();
		p.shared.override_output = c.YoungestSceneButton();
		return RequestModeUpdate();
	}
	virtual void OnSceneButtonRelease(uint8_t button) override
	{}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
		auto is_channel = c.YoungestSceneButton().has_value();
		auto chan = c.YoungestSceneButton().value_or(0);
		if (encoder == Model::EncoderAlts::Random) {
			if (is_channel) {
				p.seq.Channel(chan).IncRandomAmount(inc);
			} else {
				if (inc > 0)
					p.shared.randompool.RandomizeSequence();
				else
					p.shared.randompool.ClearSequence();
			}
			p.shared.hang.Cancel();
			return;
		} else if (encoder == Model::EncoderAlts::PlayMode) {
			if (is_channel)
				p.seq.Channel(chan).IncPlayMode(inc);
			else
				p.seq.Global().IncPlayMode(inc);
			p.shared.hang.Cancel();
			return;
		} else if (encoder == Model::EncoderAlts::ClockDiv) {
			if (!is_channel && c.toggle.trig_sense.is_high()) {
				p.internalclock.bpm_inc(inc, false);
				p.seq.Global().IncClockDiv(-100); // reset global clock div
				return;
			}
		}
		// this makes it so the value isnt affected on the first encoder click
		const auto time_now = HAL_GetTick();
		const auto hang = p.shared.hang.Check(time_now);
		inc = hang.has_value() ? inc : 0;
		p.shared.hang.Set(encoder, time_now);

		if (encoder == Model::EncoderAlts::StartOffset) {
			if (is_channel)
				p.seq.Channel(chan).IncStartOffset(inc);
			else
				p.seq.Global().IncStartOffset(inc);
		} else if (encoder == Model::EncoderAlts::PhaseOffset) {
			if (is_channel) {
				const auto len = p.seq.Channel(chan).GetLength().value_or(p.seq.Global().GetLength());
				const auto i = static_cast<float>(inc) / (len == 1 ? 1 : len - 1);
				p.seq.Channel(chan).IncPhaseOffset(i);
			} else {
				const auto len = p.seq.Global().GetLength();
				const auto i = static_cast<float>(inc) / (len == 1 ? 1 : len - 1);
				p.seq.Global().IncPhaseOffset(i);
			}
		} else if (encoder == Model::EncoderAlts::SeqLength) {
			if (is_channel)
				p.seq.Channel(chan).IncLength(inc);
			else
				p.seq.Global().IncLength(inc);
		} else if (encoder == Model::EncoderAlts::ClockDiv) {
			if (is_channel)
				p.seq.Channel(chan).IncClockDiv(inc);
			else
				p.seq.Global().IncClockDiv(inc);
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearButtonLeds();
		c.ClearEncoderLeds();

		const auto time_now = HAL_GetTick();
		auto hang = p.shared.hang.Check(time_now);

		auto length = std::make_optional(p.seq.Global().GetLength());
		auto phaseoffset = std::make_optional(p.seq.Global().GetPhaseOffset());
		auto startoffset = std::make_optional(p.seq.Global().GetStartOffset());
		auto playmode = std::make_optional(p.seq.Global().GetPlayMode());
		auto clockdiv = std::make_optional(p.seq.Global().GetClockDiv());

		auto random = 1.f;

		if (c.YoungestSceneButton().has_value()) {
			auto &chan = p.seq.Channel(c.YoungestSceneButton().value());
			length = chan.GetLength();
			phaseoffset = chan.GetPhaseOffset();
			startoffset = chan.GetStartOffset();
			playmode = chan.GetPlayMode();
			clockdiv = chan.GetClockDiv();
			random = chan.GetRandomAmount();
		}

		if (hang.has_value()) {
			if (hang.value() == Model::EncoderAlts::StartOffset) {
				if (startoffset.has_value()) {
					auto l = startoffset.value() % Model::SeqStepsPerPage;
					c.SetEncoderLed(l, Palette::magenta);
					l = startoffset.value() / Model::SeqStepsPerPage;
					c.SetButtonLed(l, true);
				} else {
					c.SetEncoderLed(Model::EncoderAlts::StartOffset, Palette::globalsetting);
				}
			} else if (hang.value() == Model::EncoderAlts::SeqLength) {
				if (length.has_value()) {
					auto l = length.value() % Model::SeqStepsPerPage;
					l = l == 0 ? Model::SeqStepsPerPage : l;
					c.SetEncoderLedsCount(l, 0, Palette::magenta);
					l = (length.value() - 1) / Model::SeqStepsPerPage;
					c.SetButtonLedsCount(l + 1, true);
				} else {
					c.SetEncoderLed(Model::EncoderAlts::SeqLength, Palette::globalsetting);
				}
			} else if (hang.value() == Model::EncoderAlts::ClockDiv) {
				if (clockdiv.has_value()) {
					const auto div = clockdiv.value();
					c.SetEncoderLedsAddition(ClockDivider::GetDivFromIdx(div), Palette::blue);
				} else {
					c.SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::globalsetting);
				}
			} else if (hang.value() == Model::EncoderAlts::PhaseOffset) {
				if (phaseoffset.has_value()) {
					PhaseOffsetDisplay(phaseoffset.value());
				} else {
					c.SetEncoderLed(Model::EncoderAlts::PhaseOffset, Palette::globalsetting);
				}
			}
		} else {
			auto col = Palette::globalsetting;
			if (phaseoffset.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::StartOffset, col);

			col = Palette::globalsetting;
			if (playmode.has_value())
				PlayModeLedAnnimation(playmode.value(), time_now);
			else
				c.SetEncoderLed(Model::EncoderAlts::PlayMode, col);

			if (length.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::SeqLength, col);

			col = Palette::globalsetting;
			if (c.toggle.trig_sense.is_high() && !c.YoungestSceneButton().has_value()) {
				if (p.internalclock.Peak())
					col = Palette::bpm;
				else
					col = Palette::off;
			} else {
				if (clockdiv.has_value())
					col = Palette::seqhead;
			}
			c.SetEncoderLed(Model::EncoderAlts::ClockDiv, col);

			if (random == 0.f)
				col = Palette::green;
			else
				col = Palette::off.blend(Palette::from_raw(p.shared.randompool.GetSeedSequence(
											 p.shared.override_output.value_or(p.GetSelectedSequence()))),
										 random);

			c.SetEncoderLed(Model::EncoderAlts::Random, col);
		}
	}

private:
	void PlayModeLedAnnimation(Sequencer::PlayMode pm, uint32_t time_now)
	{
		auto phase = (time_now & 1023) / 1023.f;
		Color col;

		if (pm == Sequencer::PlayMode::Forward) {
			col = Palette::off.blend(Palette::blue, phase);
		} else if (pm == Sequencer::PlayMode::Backward) {
			col = Palette::red.blend(Palette::off, phase);
		} else if (pm == Sequencer::PlayMode::PingPong) {
			if (phase < .5f) {
				phase *= 2.f;
				col = Palette::red.blend(Palette::blue, phase);
			} else {
				phase -= .5f;
				phase *= 2.f;
				col = Palette::blue.blend(Palette::red, phase);
			}
		} else {
			col = Palette::from_raw(time_now >> 6);
		}
		c.SetEncoderLed(Model::EncoderAlts::PlayMode, col);
	}

	void PhaseOffsetDisplay(float phase)
	{
		const auto l = (phase * 7);
		phase = l - static_cast<uint8_t>(l);
		const uint8_t mainled = static_cast<uint8_t>(l);
		uint8_t ledR = mainled + 1;

		if (mainled != 7)
			c.SetEncoderLed(ledR, Palette::off.blend(Palette::seqhead, phase));

		c.SetEncoderLed(mainled, Palette::seqhead.blend(Palette::off, phase));
	}

	bool RequestModeUpdate()
	{
		if (!c.button.shift.is_high())
			return true;

		return false;
	}
};

class MacroSettings : public Macro {
public:
	using Macro::Macro;
	void Init() override
	{
		p.shared.hang.Cancel();
	}
	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

private:
	bool RequestModeUpdate()
	{
		if (!c.button.shift.is_high())
			return true;

		return false;
	}
};

class Interface {
	Outputs outputs;
	Params &params;
	Controls controls;
	bool leds_ready_flag = false;

	mdrivlib::Timekeeper encoder_led_update_task;
	mdrivlib::Timekeeper muxio_update_task;

	Abstract *ui;

	Macro macro{params.macro, controls};
	MacroBank macrobank{params.macro, controls};
	MacroMorph macromorph{params.macro, controls};
	MacroAdd macroadd{params.macro, controls};
	MacroSettings macrosettings{params.macro, controls};

	Seq sequencer{params.sequencer, controls};
	SeqBank seqbank{params.sequencer, controls};
	SeqMorph seqmorph{params.sequencer, controls};
	SeqSettings seqsettings{params.sequencer, controls};

public:
	Interface(Params &params)
		: params{params}
	{
		encoder_led_update_task.init(Board::encoder_led_task, [this]() {
			controls.WriteToEncoderLeds();
			leds_ready_flag = true;
		});
		muxio_update_task.init(Board::muxio_conf, [this]() { controls.UpdateMuxio(); });
	}
	void Start()
	{
		encoder_led_update_task.start();
		muxio_update_task.start();
		controls.Start();
		HAL_Delay(2);
		std::srand(controls.ReadSlider() + controls.ReadCv());
		ui = &macro;
	}
	void Update()
	{
		controls.Update();

		if (ui->Update()) {
			UpdateInterface();
			ui->Init();
		}

		for (auto [i, sb] : countzip(controls.button.scene)) {
			if (sb.just_went_low())
				ui->OnSceneButtonRelease(i);
		}

		controls.ForEachEncoderInc([this](uint8_t encoder, int32_t dir) { ui->OnEncoderInc(encoder, dir); });
	}

	void SetOutputs(const Model::OutputBuffer &outs)
	{
		outputs.write(outs);

		if (!leds_ready_flag)
			return;

		leds_ready_flag = false;

		ui->PaintLeds(outs);
	}

private:
	void UpdateInterface()
	{
		if (controls.toggle.mode.is_high())
			params.mode = Params::Mode::Sequencer;
		else
			params.mode = Params::Mode::Macro;

		if (params.mode == Params::Mode::Macro)
			UpdateInterfaceMacro();
		else
			UpdateInterfaceSeq();
	}
	void UpdateInterfaceMacro()
	{
		if (controls.button.bank.is_high() && controls.button.shift.is_high())
			; // ui = &globalsettings;
		else if (controls.button.bank.is_high())
			ui = &macrobank;
		else if (controls.button.shift.is_high())
			;
		else if (controls.button.add.is_high())
			ui = &macroadd;
		else if (controls.button.morph.is_high())
			ui = &macromorph;
		else
			ui = &macro;
	}
	void UpdateInterfaceSeq()
	{
		if (controls.button.bank.is_high() && controls.button.shift.is_high())
			; // ui = &globalsettings;
		else if (controls.button.bank.is_high())
			ui = &seqbank;
		else if (controls.button.shift.is_high())
			ui = &seqsettings;
		else if (controls.button.morph.is_high())
			ui = &seqmorph;
		else
			ui = &sequencer;
	}
};

} // namespace Catalyst2::Ui
