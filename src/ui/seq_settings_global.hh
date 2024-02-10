#pragma once

#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "seq_song_mode.hh"

namespace Catalyst2::Ui::Sequencer::Settings
{
class Global : public Usual {
	SongMode songmode{p, c};

public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		c.button.play.clear_events();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		if (!c.button.shift.is_high() || c.button.bank.is_high()) {
			return;
		}

		if (p.shared.youngest_scene_button.has_value()) {
			return;
		}

		if (c.button.morph.is_high()) {
			return;
		}

		if (c.button.play.just_went_high()) {
			interface = &songmode;
			return;
		}

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		switch (encoder) {
			case Model::SeqEncoderAlts::Transpose:
				p.slot.settings.IncTranspose(inc);
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::Random:
				if (!c.button.fine.is_high()) {
					p.slot.settings.IncRandom(inc);
				} else {
					if (inc > 0) {
						//	p.randompool.Randomize();
					} else {
						//	p.randompool.Clear();
					}
				}
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::PlayMode:
				p.slot.settings.IncPlayMode(inc);
				p.player.randomsteps.Randomize();
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncStartOffset(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncPhaseOffset(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncLength(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::Range:
				break;
			case Model::SeqEncoderAlts::ClockDiv:
				if (p.seqclock.IsInternal()) {
					p.seqclock.Inc(inc, c.button.fine.is_high());
					p.shared.hang.Cancel();
				} else {
					inc = hang.has_value() ? inc : 0;
					p.shared.hang.Set(encoder, time_now);
					p.slot.clockdiv.Inc(inc);
				}
				break;
		}
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		auto clockdiv = p.slot.clockdiv;
		auto length = p.slot.settings.GetLength();
		auto startoffset = p.slot.settings.GetStartOffset();
		auto playmode = p.slot.settings.GetPlayMode();
		auto tpose = p.slot.settings.GetTranspose();
		auto random = p.slot.settings.GetRandom();

		using namespace Model;
		namespace Setting = Palette::Setting;

		if (hang.has_value()) {
			switch (hang.value()) {
				case SeqEncoderAlts::StartOffset: {
					c.SetEncoderLed(startoffset % SeqStepsPerPage, Setting::active);
					c.SetButtonLed(startoffset / SeqStepsPerPage, true);
					break;
				}
				case SeqEncoderAlts::SeqLength: {
					auto led = length % SeqStepsPerPage;
					SetEncoderLedsCount(c, led == 0 ? SeqStepsPerPage : led, 0, Setting::active);
					led = (length - 1) / SeqStepsPerPage;
					SetButtonLedsCount(c, led + 1, true);
					break;
				}
				case SeqEncoderAlts::ClockDiv: {
					SetLedsClockDiv(c, clockdiv.Read());
					break;
				}
				case SeqEncoderAlts::PhaseOffset: {
					const auto o = p.player.GetFirstStep(p.GetSelectedChannel());
					c.SetEncoderLed(o % SeqStepsPerPage, Setting::active);
					c.SetButtonLed((o / SeqStepsPerPage) % SeqPages, true);
					break;
				}
				case SeqEncoderAlts::Range: {
					break;
				}
			}
		} else {
			c.SetEncoderLed(SeqEncoderAlts::StartOffset, Setting::active);
			c.SetEncoderLed(SeqEncoderAlts::SeqLength, Setting::active);
			c.SetEncoderLed(SeqEncoderAlts::PhaseOffset, Setting::active);

			auto col = Palette::off.blend(tpose > 0 ? Setting::Transpose::positive : Setting::Transpose::negative,
										  std::abs(tpose / static_cast<float>(Transposer::max)));
			c.SetEncoderLed(SeqEncoderAlts::Transpose, col);

			PlayModeLedAnnimation(c, playmode, time_now);

			if (p.seqclock.IsInternal()) {
				if (p.seqclock.Peek()) {
					col = Setting::bpm;
				} else {
					col = Palette::off;
				}
			} else {
				col = Setting::active;
			}
			c.SetEncoderLed(SeqEncoderAlts::ClockDiv, col);

			if (c.button.fine.is_high()) {
				//	col = Palette::Random::color(p.randompool.GetSeed());
			} else {
				col = Palette::off.blend(Palette::Random::set, random);
			}
			c.SetEncoderLed(SeqEncoderAlts::Random, col);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer::Settings
