#pragma once

#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui::Settings
{
class Global : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		if (!c.button.shift.is_high() || c.button.bank.is_high()) {
			p.shared.reset.Notify(false);
			return;
		}
		if (p.shared.reset.Check()) {
			return;
		}
		if (p.shared.modeswitcher.Check()) {
			interface = nullptr;
			p.shared.data.mode = Model::Mode::Macro;
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto hang = p.shared.hang.Check();

		switch (encoder) {
			case Model::SeqEncoderAlts::Transpose:
				p.data.settings.IncTranspose(inc);
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::Random:
				if (!c.button.fine.is_high()) {
					p.data.settings.IncRandom(inc);
				} else {
					if (inc > 0) {
						p.randompool.Randomize();
					} else {
						p.randompool.Clear();
					}
				}
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::PlayMode:
				p.data.settings.IncPlayMode(inc);
				p.player.RandomizeSteps();
				p.shared.hang.Cancel();
				break;
			case Model::SeqEncoderAlts::StartOffset:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncStartOffset(inc);
				p.shared.hang.Set(encoder);
				break;
			case Model::SeqEncoderAlts::PhaseOffset:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncPhaseOffset(inc);
				p.shared.hang.Set(encoder);
				break;
			case Model::SeqEncoderAlts::SeqLength:
				inc = hang.has_value() ? inc : 0;
				p.data.settings.IncLength(inc);
				p.shared.hang.Set(encoder);
				break;
			case Model::SeqEncoderAlts::Range:
				break;
			case Model::SeqEncoderAlts::ClockDiv:
				if (p.shared.internalclock.IsInternal()) {
					p.shared.data.bpm.Inc(inc, c.button.fine.is_high());
					p.shared.hang.Cancel();
				} else {
					inc = hang.has_value() ? inc : 0;
					p.shared.hang.Set(encoder);
					p.shared.data.clockdiv.Inc(inc);
				}
				break;
		}
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check();

		auto clockdiv = p.shared.data.clockdiv;
		auto length = p.data.settings.GetLength();
		auto startoffset = p.data.settings.GetStartOffset();
		auto playmode = p.data.settings.GetPlayMode();
		auto tpose = p.data.settings.GetTranspose();
		auto random = p.data.settings.GetRandom();

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
					SetEncoderLedsCount(led == 0 ? SeqStepsPerPage : led, 0, Setting::active);
					led = (length - 1) / SeqStepsPerPage;
					SetButtonLedsCount(led + 1, true);
					break;
				}
				case SeqEncoderAlts::ClockDiv: {
					SetEncoderLedsAddition(clockdiv.Read(), Setting::clockdiv);
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

			PlayModeLedAnnimation(playmode, time_now);

			if (p.shared.internalclock.IsInternal()) {
				if (p.shared.internalclock.Peek()) {
					col = Setting::bpm;
				} else {
					col = Palette::off;
				}
			} else {
				col = Setting::active;
			}
			c.SetEncoderLed(SeqEncoderAlts::ClockDiv, col);

			if (c.button.fine.is_high()) {
				col = Palette::Random::color(p.randompool.GetSeed());
			} else {
				col = Palette::off.blend(Palette::Random::set, random);
			}
			c.SetEncoderLed(SeqEncoderAlts::Random, col);
		}
	}
};

} // namespace Catalyst2::Sequencer::Ui::Settings
