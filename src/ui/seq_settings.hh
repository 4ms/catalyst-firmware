#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer::Settings
{
class Channel : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		if (!p.IsChannelSelected()) {
			p.SelectChannel();
		}
		p.shared.modeswitcher.SetAlarm(p.shared.internalclock.TimeNow());
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.add.is_high()) {
			p.shared.modeswitcher.SetAlarm(p.shared.internalclock.TimeNow());
		}

		if (!c.button.shift.is_high() || !c.button.bank.is_high()) {
			return;
		}

		if (p.shared.modeswitcher.Check(p.shared.internalclock.TimeNow())) {
			p.shared.mode = Model::Mode::Macro;
			for (auto i = 0u; i < Model::NumChans; i++) {
				p.shared.blinker.Set(i, 1, 200, p.shared.internalclock.TimeNow(), 100 * i + 250);
			}
			return;
		}
		interface = this;
	}
	void OnSceneButtonRelease(uint8_t scene) {
		p.shared.hang.Cancel();
		if (scene != p.GetSelectedChannel()) {
			p.SelectChannel(scene);
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());

		const auto i = p.GetSelectedChannel();
		const auto time_now = p.shared.internalclock.TimeNow();

		switch (encoder) {
			case Model::Sequencer::EncoderAlts::Transpose:
				p.slot.settings.IncTranspose(i, inc);
				p.shared.hang.Cancel();
				break;
			case Model::Sequencer::EncoderAlts::Random:
				p.slot.settings.IncRandom(i, inc);
				p.shared.hang.Cancel();
				break;
			case Model::Sequencer::EncoderAlts::PlayMode:
				p.slot.settings.IncPlayMode(i, inc);
				p.player.randomsteps.Randomize(i);
				p.shared.hang.Cancel();
				break;
			case Model::Sequencer::EncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncStartOffset(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::Sequencer::EncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncPhaseOffset(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::Sequencer::EncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncLength(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::Sequencer::EncoderAlts::Range:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncRange(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::Sequencer::EncoderAlts::ClockDiv:
				inc = hang.has_value() ? inc : 0;
				p.slot.settings.IncClockDiv(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		c.SetButtonLed(p.GetSelectedChannel(), true);

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		const auto chan = p.GetSelectedChannel();
		const auto length = p.slot.settings.GetLength(chan);
		const auto phaseoffset = p.slot.settings.GetPhaseOffset(chan);
		const auto startoffset = p.slot.settings.GetStartOffset(chan);
		const auto playmode = p.slot.settings.GetPlayMode(chan);
		const auto clockdiv = p.slot.settings.GetClockDiv(chan);
		const auto tpose = p.slot.settings.GetTranspose(chan);
		const auto random = p.slot.settings.GetRandom(chan);
		const auto range = p.slot.settings.GetRange(chan);

		using namespace Model::Sequencer;
		namespace Setting = Palette::Setting;

		if (hang.has_value()) {
			ClearButtonLeds(c);
			switch (hang.value()) {
				case EncoderAlts::StartOffset: {
					const auto l = startoffset.value_or(p.slot.settings.GetStartOffset());
					const auto col = startoffset.has_value() ? Setting::active : Setting::null;
					c.SetEncoderLed(l % Steps::PerPage, col);
					c.SetButtonLed(l / Steps::PerPage, true);
					break;
				}
				case EncoderAlts::SeqLength: {
					const auto l = length.value_or(p.slot.settings.GetLength());
					const auto col = length.has_value() ? Setting::active : Setting::null;
					auto led = l % Steps::PerPage;
					SetEncoderLedsCount(c, led == 0 ? Steps::PerPage : led, 0, col);
					led = (l - 1) / Steps::PerPage;
					SetButtonLedsCount(c, led + 1, true);
					break;
				}
				case EncoderAlts::ClockDiv: {
					SetLedsClockDiv(c, clockdiv.Read());
					break;
				}
				case EncoderAlts::PhaseOffset: {
					const auto o = p.player.GetFirstStep(chan);
					const auto col = phaseoffset.has_value() ? Setting::active : Setting::null;
					c.SetEncoderLed(o % Steps::PerPage, col);
					c.SetButtonLed((o / Steps::PerPage) % NumPages, true);
					break;
				}
				case EncoderAlts::Range: {
					DisplayRange(c, range);
					break;
				}
			}
		} else {
			auto col = startoffset.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(EncoderAlts::StartOffset, col);

			col =
				tpose.has_value() ?
					Palette::off.blend(tpose.value() > 0 ? Setting::Transpose::positive : Setting::Transpose::negative,
									   std::abs(tpose.value()) / static_cast<float>(Transposer::max)) :
					Setting::null;
			c.SetEncoderLed(EncoderAlts::Transpose, col);

			if (playmode.has_value()) {
				PlayModeLedAnimation(c, playmode.value(), time_now);
			} else {
				c.SetEncoderLed(EncoderAlts::PlayMode, Setting::null);
			}

			col = length.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(EncoderAlts::SeqLength, col);

			col = phaseoffset.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(EncoderAlts::PhaseOffset, col);

			c.SetEncoderLed(EncoderAlts::ClockDiv, Setting::active);

			col = random.has_value() ? Palette::off.blend(Palette::Random::set, random.value()) : Setting::null;
			if (c.button.fine.is_high()) {
				// col = Palette::Random::color(p.randompool.GetSeed());
			}
			c.SetEncoderLed(EncoderAlts::Random, col);

			c.SetEncoderLed(EncoderAlts::Range, Setting::active);
		}
	}
};
} // namespace Catalyst2::Ui::Sequencer::Settings
