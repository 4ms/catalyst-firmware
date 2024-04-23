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
	SongMode songmode{p, c, &main_ui};

public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		c.button.play.clear_events();
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		if (!c.button.shift.is_high() || c.button.bank.is_high() || c.button.morph.is_high() ||
			p.shared.youngest_scene_button.has_value())
		{
			SwitchUiMode(main_ui);
			return;
		}

		if (c.button.play.just_went_high()) {
			SwitchUiMode(songmode);
			return;
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto hang = p.shared.hang.Check();

		const auto fine = c.button.fine.is_high();

		switch (encoder) {
			using namespace Model::Sequencer;
			case EncoderAlts::Transpose:
				p.slot.settings.IncTranspose(inc, fine);
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
				p.shared.hang.Set(encoder);
				break;
			case EncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncPhaseOffset(inc);
				p.shared.hang.Set(encoder);
				break;
			case EncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncLength(inc);
				p.shared.hang.Set(encoder);
				break;
			case EncoderAlts::Range:
				break;
			case EncoderAlts::ClockDiv:
				if (c.button.add.is_high() && p.seqclock.external) {
					p.slot.clock_sync_mode.Inc(inc);
					p.shared.hang.Cancel();
				} else {
					if (p.seqclock.external) {
						inc = hang.has_value() ? inc : 0;
						p.shared.hang.Set(encoder);
						p.slot.clockdiv.Inc(inc);
					} else {
						p.seqclock.Inc(inc, fine);
						p.shared.hang.Cancel();
					}
				}
				break;
		}
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);

		using namespace Model::Sequencer;
		namespace Setting = Palette::Setting;

		if (c.button.add.is_high() && p.seqclock.external) {
			Color col;
			if (p.slot.clock_sync_mode.mode == Clock::Sync::Mode::SYNCED) {
				col = Setting::clock_mode_sync;
			} else {
				col = Setting::clock_mode_dinsync;
			}
			c.SetEncoderLed(EncoderAlts::ClockDiv, col);
		} else {

			const auto hang = p.shared.hang.Check();

			auto clockdiv = p.slot.clockdiv;
			auto length = p.slot.settings.GetLength();
			auto startoffset = p.slot.settings.GetStartOffset();
			auto playmode = p.slot.settings.GetPlayMode();
			auto tpose = p.slot.settings.GetTranspose();
			auto random = p.slot.settings.GetRandom();

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
						o += p.slot.settings.GetPhaseOffsetOrGlobal(chan) *
							 (p.slot.settings.GetLengthOrGlobal(chan) - 1);
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

				PlayModeLedAnimation(c, playmode);

				if (p.seqclock.external) {
					col = Setting::active;
				} else {
					if (p.seqclock.PeekPhase() < 0.5f) {
						col = Setting::bpm;
					} else {
						col = Palette::off;
					}
				}
				c.SetEncoderLed(EncoderAlts::ClockDiv, col);

				col = Palette::off.blend(Palette::Random::set, random);
				c.SetEncoderLed(EncoderAlts::Random, col);
			}
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer::Settings
