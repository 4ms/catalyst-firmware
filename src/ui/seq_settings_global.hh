#pragma once

#include "conf/model.hh"
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
			using namespace Model::Sequencer;
			case EncoderAlts::Transpose:
				p.slot.settings.IncTranspose(inc);
				p.shared.hang.Cancel();
				break;
			case EncoderAlts::Random:
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
			case EncoderAlts::PlayMode:
				p.slot.settings.IncPlayMode(inc);
				p.player.randomsteps.Randomize();
				p.shared.hang.Cancel();
				break;
			case EncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncStartOffset(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case EncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncPhaseOffset(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case EncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncLength(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case EncoderAlts::Range:
				break;
			case EncoderAlts::ClockDiv:
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

		using namespace Model::Sequencer;
		namespace Setting = Palette::Setting;

		if (hang.has_value()) {
			switch (hang.value()) {
				case EncoderAlts::StartOffset: {
					c.SetEncoderLed(startoffset % Steps::PerPage, Setting::active);
					c.SetButtonLed(startoffset / Steps::PerPage, true);
					break;
				}
				case EncoderAlts::SeqLength: {
					auto led = length % Steps::PerPage;
					SetEncoderLedsCount(c, led == 0 ? Steps::PerPage : led, 0, Setting::active);
					led = (length - 1) / Steps::PerPage;
					SetButtonLedsCount(c, led + 1, true);
					break;
				}
				case EncoderAlts::ClockDiv: {
					SetLedsClockDiv(c, clockdiv.Read());
					break;
				}
				case EncoderAlts::PhaseOffset: {
					auto chan = p.GetSelectedChannel();
					auto o = p.player.GetFirstStep(chan);
					o += p.slot.settings.GetPhaseOffsetOrGlobal(chan) * (p.slot.settings.GetLengthOrGlobal(chan) - 1);
					c.SetEncoderLed(o % Steps::PerPage, Setting::active);
					c.SetButtonLed((o / Steps::PerPage) % NumPages, true);
					break;
				}
				case EncoderAlts::Range: {
					break;
				}
			}
		} else {
			c.SetEncoderLed(EncoderAlts::StartOffset, Setting::active);
			c.SetEncoderLed(EncoderAlts::SeqLength, Setting::active);
			c.SetEncoderLed(EncoderAlts::PhaseOffset, Setting::active);

			auto col = Palette::off.blend(tpose > 0 ? Setting::Transpose::positive : Setting::Transpose::negative,
										  std::abs(tpose / static_cast<float>(Transposer::max)));
			c.SetEncoderLed(EncoderAlts::Transpose, col);

			PlayModeLedAnimation(c, playmode, time_now);

			if (p.seqclock.IsInternal()) {
				if (p.seqclock.Peek()) {
					col = Setting::bpm;
				} else {
					col = Palette::off;
				}
			} else {
				col = Setting::active;
			}
			c.SetEncoderLed(EncoderAlts::ClockDiv, col);

			if (c.button.fine.is_high()) {
				//	col = Palette::Random::color(p.randompool.GetSeed());
			} else {
				col = Palette::off.blend(Palette::Random::set, random);
			}
			c.SetEncoderLed(EncoderAlts::Random, col);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer::Settings
