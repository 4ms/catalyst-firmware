#pragma once

#include "../params.hh"
#include "abstract.hh"

namespace Catalyst2::Sequencer::Ui
{
class Usual : public Catalyst2::Ui::Abstract {
public:
	SeqMode::Interface &p;
	Usual(SeqMode::Interface &p, Controls &c)
		: Abstract{c}
		, p{p}
	{}
	virtual void Common() override
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

		if (c.button.play.just_went_high())
			p.seq.TogglePause();

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
};

class Bank : public Usual {
public:
	using Usual::Usual;
	virtual void Init() override
	{
		c.button.fine.clear_events();
	}
	virtual void Update(Abstract *&interface) override
	{
		if (c.button.fine.just_went_high() && p.IsSequenceSelected())
			p.seq.CopySequence(p.GetSelectedSequence());

		if (!c.button.bank.is_high())
			return;

		if (c.button.shift.is_high())
			return;

		interface = this;
	}
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
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearButtonLeds();
		c.SetButtonLed(p.GetSelectedSequence(), true);

		for (auto i = 0u; i < Model::NumChans; i++)
			c.SetEncoderLed(i, p.seq.Channel(i).mode.GetColor());
	}
};

class Morph : public Usual {
public:
	using Usual::Usual;
	virtual void Init() override
	{
		if (!p.IsSequenceSelected())
			p.SelectSequence(0);
	}
	virtual void Update(Abstract *&interface)
	{
		if (!c.button.morph.is_high())
			return;

		interface = this;
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
};

class Settings : public Usual {
public:
	using Usual::Usual;
	virtual void Init() override
	{
		p.shared.hang.Cancel();
	}
	virtual void Update(Abstract *&interface)
	{
		if (!c.button.shift.is_high())
			return;

		interface = this;
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) override
	{
		const auto time_now = HAL_GetTick();
		const auto hang = p.shared.hang.Check(time_now);

		if (!p.IsSequenceSelected()) {
			if (encoder == Model::EncoderAlts::ClockDiv) {
				inc = hang.has_value() ? inc : 0;
				p.shared.IncClockDiv(inc);
				p.shared.hang.Set(encoder, time_now);
			}
			return;
		}

		auto is_channel = c.YoungestSceneButton().has_value();
		auto chan = c.YoungestSceneButton().value_or(0);

		switch (encoder) {
			case Model::EncoderAlts::Random:
				if (is_channel) {
					p.seq.Channel(chan).IncRandomAmount(inc);
				} else {
					if (inc > 0)
						p.shared.randompool.RandomizeSequence();
					else
						p.shared.randompool.ClearSequence();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::PlayMode:
				if (is_channel) {
					p.seq.Channel(chan).IncPlayMode(inc);
					if (p.seq.Channel(chan).GetPlayMode() == Sequencer::PlayMode::Random)
						p.seq.RandomizeStepPattern(chan);
				} else {
					p.seq.Global().IncPlayMode(inc);
					if (p.seq.Global().GetPlayMode() == Sequencer::PlayMode::Random)
						p.seq.RandomizeStepPattern();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				if (is_channel)
					p.seq.Channel(chan).IncStartOffset(inc);
				else
					p.seq.Global().IncStartOffset(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				if (is_channel) {
					const auto len = p.seq.Channel(chan).GetLength().value_or(p.seq.Global().GetLength());
					const auto i = static_cast<float>(inc) / (len == 1 ? 1 : len - 1);
					p.seq.Channel(chan).IncPhaseOffset(i);
				} else {
					const auto len = p.seq.Global().GetLength();
					const auto i = static_cast<float>(inc) / (len == 1 ? 1 : len - 1);
					p.seq.Global().IncPhaseOffset(i);
				}
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				if (is_channel)
					p.seq.Channel(chan).IncLength(inc);
				else
					p.seq.Global().IncLength(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::ClockDiv:
				if (is_channel) {
					inc = hang.has_value() ? inc : 0;
					p.seq.Channel(chan).IncClockDiv(inc);
					p.shared.hang.Set(encoder, time_now);
				} else {
					if (c.toggle.trig_sense.is_high()) {
						p.internalclock.bpm_inc(inc, false);
						p.seq.Global().IncClockDiv(-100); // reset global clock div
						p.shared.hang.Cancel();
					} else {
						inc = hang.has_value() ? inc : 0;
						p.seq.Global().IncClockDiv(inc);
						p.shared.hang.Set(encoder, time_now);
					}
				}
				break;
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c.ClearButtonLeds();
		c.ClearEncoderLeds();

		const auto time_now = HAL_GetTick();
		auto hang = p.shared.hang.Check(time_now);

		auto clockdiv = std::make_optional(p.seq.Global().GetClockDiv());

		if (!p.IsSequenceSelected()) {
			clockdiv = p.shared.GetClockDiv();
			if (hang.has_value()) {
				if (hang.value() != Model::EncoderAlts::ClockDiv)
					return;
				if (clockdiv.has_value())
					c.SetEncoderLedsAddition(ClockDivider::GetDivFromIdx(clockdiv.value()), Palette::blue);
				else
					c.SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::globalsetting);
			} else
				c.SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::seqhead);
			return;
		}

		auto length = std::make_optional(p.seq.Global().GetLength());
		auto phaseoffset = std::make_optional(p.seq.Global().GetPhaseOffset());
		auto startoffset = std::make_optional(p.seq.Global().GetStartOffset());
		auto playmode = std::make_optional(p.seq.Global().GetPlayMode());
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
				if (p.internalclock.Peek())
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
};

class Main : public Usual {
	Bank bank{p, c};
	Morph morph{p, c};
	Settings settings{p, c};

public:
	using Usual::Usual;
	virtual void Init() override
	{
		c.button.fine.clear_events();
		c.button.bank.clear_events();
	}
	virtual void Update(Abstract *&interface) override
	{
		if (p.IsSequenceSelected()) {
			const auto curseq = p.GetSelectedSequence();
			if (c.button.fine.just_went_high() && c.YoungestSceneButton().has_value())
				p.seq.CopyPage(curseq, c.YoungestSceneButton().value());

			if (c.button.bank.just_went_high() && c.button.fine.is_high())
				p.seq.PasteSequence(curseq);
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
			if (c.button.fine.is_high()) {
				p.seq.PastePage(p.GetSelectedSequence(), button);
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

	void EncoderDisplayOutput(const Model::OutputBuffer &buf)
	{
		for (auto [chan, val] : countzip(buf)) {
			Color col = EncoderBlend(val, p.seq.Channel(chan).mode.IsGate());
			c.SetEncoderLed(chan, col);
		}
	}
};

} // namespace Catalyst2::Sequencer::Ui