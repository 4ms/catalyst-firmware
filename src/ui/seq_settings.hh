#pragma once

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
			p.SelectChannel(0);
		}
		p.shared.modeswitcher.SetAlarm(p.shared.internalclock.TimeNow());
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

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
		p.SelectChannel(scene);
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());

		const auto i = p.GetSelectedChannel();
		const auto time_now = p.shared.internalclock.TimeNow();

		switch (encoder) {
			case Model::SeqEncoderAlts::Transpose:
				p.data.settings.IncTranspose(i, inc);
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::Random:
				p.data.settings.IncRandom(i, inc);
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::PlayMode:
				p.data.settings.IncPlayMode(i, inc);
				p.player.randomsteps.Randomize(i);
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncStartOffset(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncPhaseOffset(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncLength(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::Range:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncRange(i, inc);
				p.shared.hang.Set(encoder, time_now);
				break;
			case Model::SeqEncoderAlts::ClockDiv:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncClockDiv(i, inc);
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
		const auto length = p.data.settings.GetLength(chan);
		const auto phaseoffset = p.data.settings.GetPhaseOffset(chan);
		const auto startoffset = p.data.settings.GetStartOffset(chan);
		const auto playmode = p.data.settings.GetPlayMode(chan);
		const auto clockdiv = p.data.settings.GetClockDiv(chan);
		const auto tpose = p.data.settings.GetTranspose(chan);
		const auto random = p.data.settings.GetRandom(chan);
		const auto range = p.data.settings.GetRange(chan);

		using namespace Model;
		namespace Setting = Palette::Setting;

		if (hang.has_value()) {
			ClearButtonLeds(c);
			switch (hang.value()) {
				case SeqEncoderAlts::StartOffset: {
					const auto l = startoffset.value_or(p.data.settings.GetStartOffset());
					const auto col = startoffset.has_value() ? Setting::active : Setting::null;
					c.SetEncoderLed(l % SeqStepsPerPage, col);
					c.SetButtonLed(l / SeqStepsPerPage, true);
					break;
				}
				case SeqEncoderAlts::SeqLength: {
					const auto l = length.value_or(p.data.settings.GetLength());
					const auto col = length.has_value() ? Setting::active : Setting::null;
					auto led = l % SeqStepsPerPage;
					SetEncoderLedsCount(c, led == 0 ? SeqStepsPerPage : led, 0, col);
					led = (l - 1) / SeqStepsPerPage;
					SetButtonLedsCount(c, led + 1, true);
					break;
				}
				case SeqEncoderAlts::ClockDiv: {
					SetLedsClockDiv(c, clockdiv.Read());
					break;
				}
				case SeqEncoderAlts::PhaseOffset: {
					const auto o = p.player.GetFirstStep(chan);
					const auto col = phaseoffset.has_value() ? Setting::active : Setting::null;
					c.SetEncoderLed(o % SeqStepsPerPage, col);
					c.SetButtonLed((o / SeqStepsPerPage) % SeqPages, true);
					break;
				}
				case SeqEncoderAlts::Range: {
					DisplayRange(c, range);
					break;
				}
			}
		} else {
			auto col = startoffset.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(SeqEncoderAlts::StartOffset, col);

			col =
				tpose.has_value() ?
					Palette::off.blend(tpose.value() > 0 ? Setting::Transpose::positive : Setting::Transpose::negative,
									   std::abs(tpose.value()) / static_cast<float>(Transposer::max)) :
					Setting::null;
			c.SetEncoderLed(SeqEncoderAlts::Transpose, col);

			if (playmode.has_value()) {
				PlayModeLedAnnimation(c, playmode.value(), time_now);
			} else {
				c.SetEncoderLed(SeqEncoderAlts::PlayMode, Setting::null);
			}

			col = length.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(SeqEncoderAlts::SeqLength, col);

			col = phaseoffset.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(SeqEncoderAlts::PhaseOffset, col);

			c.SetEncoderLed(SeqEncoderAlts::ClockDiv, Setting::active);

			col = random.has_value() ? Palette::off.blend(Palette::Random::set, random.value()) : Setting::null;
			if (c.button.fine.is_high()) {
				// col = Palette::Random::color(p.randompool.GetSeed());
			}
			c.SetEncoderLed(SeqEncoderAlts::Random, col);

			c.SetEncoderLed(SeqEncoderAlts::Range, Setting::active);
		}
	}
};
} // namespace Catalyst2::Ui::Sequencer::Settings
