#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui
{
class Settings : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
	}
	void Update(Abstract *&interface) {
		if (!c.button.shift.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		auto is_channel = c.YoungestSceneButton().has_value();
		auto chan = c.YoungestSceneButton().value_or(0);

		switch (encoder) {
			case Model::EncoderAlts::Transpose:
				if (is_channel)
					p.seq.Channel(chan).transposer.Inc(inc);
				else
					p.seq.Global().transposer.Inc(inc);
				p.shared.hang.Cancel();
				break;
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
					p.seq.Channel(chan).playmode.Inc(inc);
					if (p.seq.Channel(chan).playmode.Read() == Sequencer::PlayMode::Random)
						p.seq.player.RandomizeSteps(chan);
				} else {
					p.seq.Global().playmode.Inc(inc);
					if (p.seq.Global().playmode.Read() == Sequencer::PlayMode::Random)
						p.seq.player.RandomizeSteps();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				if (is_channel)
					p.seq.Channel(chan).start_offset.Inc(inc);
				else
					p.seq.Global().start_offset.Inc(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				if (is_channel) {
					const auto len = p.seq.Channel(chan).length.Read().value_or(p.seq.Global().length.Read().value());
					const auto i = static_cast<float>(inc) / (len == 1 ? 1 : len - 1);
					p.seq.Channel(chan).phase_offset.Inc(i);
				} else {
					const auto len = p.seq.Global().length.Read().value();
					const auto i = static_cast<float>(inc) / (len == 1 ? 1 : len - 1);
					p.seq.Global().phase_offset.Inc(i);
				}
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				if (is_channel)
					p.seq.Channel(chan).length.Inc(inc);
				else
					p.seq.Global().length.Inc(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::ClockDiv:
				if (is_channel) {
					inc = hang.has_value() ? inc : 0;
					p.seq.Channel(chan).IncClockDiv(inc);
					p.shared.hang.Set(encoder, time_now);
				} else {
					if (c.toggle.trig_sense.is_high()) {
						p.shared.internalclock.Inc(inc, c.button.fine.is_high());
						p.shared.hang.Cancel();
					} else {
						inc = hang.has_value() ? inc : 0;
						p.shared.hang.Set(encoder, time_now);
						p.shared.clockdiv.Inc(inc);
					}
				}
				break;
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();
		c.ClearEncoderLeds();

		const auto time_now = p.shared.internalclock.TimeNow();
		auto hang = p.shared.hang.Check(time_now);

		auto clockdiv = p.shared.clockdiv;

		auto length = p.seq.Global().length.Read();
		auto phaseoffset = p.seq.Global().phase_offset.Read();
		auto startoffset = p.seq.Global().start_offset.Read();
		auto playmode = p.seq.Global().playmode.Read();
		auto tpose = p.seq.Global().transposer.Read();
		auto random = p.shared.randompool.IsRandomized() ? 1.f : 0.f;

		if (c.YoungestSceneButton().has_value()) {
			auto &chan = p.seq.Channel(c.YoungestSceneButton().value());
			length = chan.length.Read();
			phaseoffset = chan.phase_offset.Read();
			startoffset = chan.start_offset.Read();
			playmode = chan.playmode.Read();
			clockdiv = chan.GetClockDiv();
			tpose = chan.transposer.Read();
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
				c.SetEncoderLedsAddition(clockdiv.Read(), Palette::blue);
			} else if (hang.value() == Model::EncoderAlts::PhaseOffset) {
				if (phaseoffset.has_value())
					PhaseOffsetDisplay(phaseoffset.value());
				else
					c.SetEncoderLed(Model::EncoderAlts::PhaseOffset, Palette::globalsetting);
			}
		} else {
			auto col = Palette::globalsetting;
			if (phaseoffset.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::StartOffset, col);

			col = Palette::globalsetting;
			if (tpose.has_value())
				col = Palette::off.blend(Palette::green, tpose.value() / 12.f);
			c.SetEncoderLed(Model::EncoderAlts::Transpose, col);

			col = Palette::globalsetting;
			if (playmode.has_value())
				PlayModeLedAnnimation(playmode.value(), time_now);
			else
				c.SetEncoderLed(Model::EncoderAlts::PlayMode, col);

			if (length.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::SeqLength, col);

			col = Palette::globalsetting;
			if (phaseoffset.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::PhaseOffset, col);

			col = Palette::globalsetting;
			if (c.toggle.trig_sense.is_high() && !c.YoungestSceneButton().has_value()) {
				if (p.shared.internalclock.Peek())
					col = Palette::bpm;
				else
					col = Palette::off;
			} else {
				col = Palette::seqhead;
			}
			c.SetEncoderLed(Model::EncoderAlts::ClockDiv, col);

			if (!p.shared.randompool.IsRandomized() || random == 0.f)
				col = Palette::red;
			else
				col = Palette::off.blend(Palette::from_raw(p.shared.randompool.GetSeedSequence(
											 c.YoungestSceneButton().value_or(p.GetSelectedSequence()))),
										 random);

			c.SetEncoderLed(Model::EncoderAlts::Random, col);
		}
	}

private:
	void PlayModeLedAnnimation(Sequencer::PlayMode pm, uint32_t time_now) {
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

	void PhaseOffsetDisplay(float phase) {
		const auto l = (phase * 7);
		phase = l - static_cast<uint8_t>(l);
		const uint8_t mainled = static_cast<uint8_t>(l);
		uint8_t ledR = mainled + 1;

		if (mainled != 7)
			c.SetEncoderLed(ledR, Palette::off.blend(Palette::seqhead, phase));

		c.SetEncoderLed(mainled, Palette::seqhead.blend(Palette::off, phase));
	}
};

} // namespace Catalyst2::Sequencer::Ui