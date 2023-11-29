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
	void Update(Abstract *&interface) override {
		if (!c.button.shift.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);
		const auto ysb = YoungestSceneButton();

		switch (encoder) {
			case Model::EncoderAlts::Transpose:
				if (ysb.has_value()) {
					p.data.settings.IncTranspose(ysb.value(), inc);
				} else {
					p.data.settings.IncTranspose(inc);
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::Random:
				if (ysb.has_value()) {
					p.data.settings.IncRandomAmount(ysb.value(), inc);
				} else {
					if (inc > 0)
						p.shared.randompool.RandomizeSequence();
					else
						p.shared.randompool.ClearSequence();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::PlayMode:
				if (ysb.has_value()) {
					p.data.settings.IncPlayMode(ysb.value(), inc);
					p.player.RandomizeSteps(ysb.value());
				} else {
					p.data.settings.IncPlayMode(inc);
					p.player.RandomizeSteps();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				if (ysb.has_value()) {
					p.data.settings.IncStartOffset(ysb.value(), inc);
				} else {
					p.data.settings.IncStartOffset(inc);
				}
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				if (ysb.has_value()) {
					p.data.settings.IncPhaseOffset(ysb.value(), inc);
				} else {
					p.data.settings.IncPhaseOffset(inc);
				}
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				if (ysb.has_value()) {
					p.data.settings.IncLength(ysb.value(), inc);
				} else {
					p.data.settings.IncLength(inc);
				}
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::EncoderAlts::Range:
				if (ysb.has_value()) {
					inc = hang.has_value() ? inc : 0;
					p.data.settings.IncRange(ysb.value(), inc);
					p.UpdateRange(ysb.value());
					p.shared.hang.Set(encoder, time_now);
				}
				break;
			case Model::EncoderAlts::ClockDiv:
				if (ysb.has_value()) {
					inc = hang.has_value() ? inc : 0;
					p.data.settings.IncClockDiv(ysb.value(), inc);
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
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		auto clockdiv = p.shared.clockdiv;

		auto length = std::make_optional(p.data.settings.GetLength());
		auto phaseoffset = std::make_optional(p.data.settings.GetPhaseOffset());
		auto startoffset = std::make_optional(p.data.settings.GetStartOffset());
		auto playmode = std::make_optional(p.data.settings.GetPlayMode());
		auto tpose = std::make_optional(p.data.settings.GetTranspose());
		auto range = Channel::Range{};
		auto random = p.shared.randompool.IsRandomized() ? 1.f : 0.f;

		auto ysb = YoungestSceneButton();
		if (ysb.has_value()) {
			const auto chan = ysb.value();
			length = p.data.settings.GetLength(chan);
			phaseoffset = p.data.settings.GetPhaseOffset(chan);
			startoffset = p.data.settings.GetStartOffset(chan);
			playmode = p.data.settings.GetPlayMode(chan);
			clockdiv = p.data.settings.GetClockDiv(chan);
			tpose = p.data.settings.GetTranspose(chan);
			random = p.data.settings.GetRandomAmount(chan);
			range = p.data.settings.GetRange(chan);
		}

		if (hang.has_value()) {
			switch (hang.value()) {
				using namespace Model;
				case EncoderAlts::StartOffset: {
					const auto l = startoffset.value_or(p.data.settings.GetStartOffset());
					const auto col = startoffset.has_value() ? Palette::Setting::active : Palette::Setting::null;
					c.SetEncoderLed(l % Model::SeqStepsPerPage, col);
					c.SetButtonLed(l / Model::SeqStepsPerPage, true);
				} break;
				case EncoderAlts::SeqLength: {
					const auto l = length.value_or(p.data.settings.GetLength());
					const auto col = length.has_value() ? Palette::Setting::active : Palette::Setting::null;
					auto led = l % Model::SeqStepsPerPage;
					SetEncoderLedsCount(led == 0 ? Model::SeqStepsPerPage : led, 0, col);
					led = (l - 1) / Model::SeqStepsPerPage;
					SetButtonLedsCount(led + 1, true);
				} break;
				case EncoderAlts::ClockDiv: {
					SetEncoderLedsAddition(clockdiv.Read(), Palette::blue);
				} break;
				case EncoderAlts::PhaseOffset: {
					const auto o = p.player.GetFirstStep(ysb, p.shared.GetPos());
					const auto col = phaseoffset.has_value() ? Palette::Setting::active : Palette::Setting::null;
					c.SetEncoderLed(o % Model::SeqStepsPerPage, col);
					c.SetButtonLed((o / Model::SeqStepsPerPage) % Model::SeqPages, true);
				} break;
				case EncoderAlts::Range: {
					const auto neg = range.NegAmount();
					const auto pos = range.PosAmount();
					const auto posleds = static_cast<uint8_t>(pos * (Model::NumChans / 2u));
					const auto negleds = static_cast<uint8_t>(neg * (Model::NumChans / 2u));
					const auto lastposledfade = pos * (Model::NumChans / 2u) - posleds;
					const auto lastnegledfade = neg * (Model::NumChans / 2u) - negleds;
					for (auto i = 0u; i < posleds; i++) {
						c.SetEncoderLed(i + (Model::NumChans / 2u), Palette::Voltage::Positive);
					}
					c.SetEncoderLed((Model::NumChans / 2u) + posleds,
									Palette::off.blend(Palette::Voltage::Positive, lastposledfade));
					for (auto i = 0u; i < negleds; i++) {
						c.SetEncoderLed((Model::NumChans / 2u) - 1 - i, Palette::Voltage::Negative);
					}
					c.SetEncoderLed((Model::NumChans / 2u) - negleds - 1,
									Palette::off.blend(Palette::Voltage::Negative, lastnegledfade));
				} break;
			}
		} else {
			auto col = Palette::Setting::null;
			if (startoffset.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::StartOffset, col);

			col = Palette::Setting::null;
			if (tpose.has_value())
				col = Palette::off.blend(Palette::green, tpose.value() / 12.f);
			c.SetEncoderLed(Model::EncoderAlts::Transpose, col);

			col = Palette::Setting::null;
			if (playmode.has_value())
				PlayModeLedAnnimation(playmode.value(), time_now);
			else
				c.SetEncoderLed(Model::EncoderAlts::PlayMode, col);

			if (length.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::SeqLength, col);

			col = Palette::Setting::null;
			if (phaseoffset.has_value())
				col = Palette::seqhead;
			c.SetEncoderLed(Model::EncoderAlts::PhaseOffset, col);

			col = Palette::Setting::null;
			if (c.toggle.trig_sense.is_high() && !ysb.has_value()) {
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
				col = Palette::off.blend(
					Palette::from_raw(p.shared.randompool.GetSeedSequence(ysb.value_or(p.GetSelectedChannel()))),
					random);

			c.SetEncoderLed(Model::EncoderAlts::Random, col);
		}
	}

private:
	void PlayModeLedAnnimation(Sequencer::PlayMode pm, uint32_t time_now) {
		static constexpr auto animation_duration = static_cast<float>(Clock::MsToTicks(1000));
		auto phase = (time_now / animation_duration);
		phase -= static_cast<uint32_t>(phase);
		Color col;

		if (pm == Sequencer::PlayMode::Forward) {
			col = Palette::off.blend(Palette::blue, phase);
		} else if (pm == Sequencer::PlayMode::Backward) {
			col = Palette::red.blend(Palette::off, phase);
		} else if (pm == Sequencer::PlayMode::PingPong) {
			if (phase < .5f) {
				phase *= 2.f;
				col = Palette::red.blend(Palette::off, phase);
			} else {
				phase -= .5f;
				phase *= 2.f;
				col = Palette::blue.blend(Palette::off, phase);
			}
		} else {
			col = Palette::from_raw(time_now >> 8);
		}
		c.SetEncoderLed(Model::EncoderAlts::PlayMode, col);
	}
};

} // namespace Catalyst2::Sequencer::Ui
