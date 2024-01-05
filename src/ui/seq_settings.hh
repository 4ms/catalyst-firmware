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
		if (!c.button.shift.is_high()) {
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
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto hang = p.shared.hang.Check();
		const auto ysb = YoungestSceneButton();

		for (auto [i, b] : countzip(c.button.scene)) {
			if (ysb.has_value()) {
				if (!b.is_high()) {
					continue;
				}
			} else {
				if (i != 0) {
					continue;
				}
			}

			switch (encoder) {
				case Model::EncoderAlts::Transpose:
					if (ysb.has_value()) {
						p.data.settings.IncTranspose(i, inc);
					} else {
						p.data.settings.IncTranspose(inc);
					}
					p.shared.hang.Cancel();
					break;
				case Model::EncoderAlts::Random:
					if (ysb.has_value()) {
						p.data.settings.IncRandomAmount(i, inc);
					} else {
						if (inc > 0) {
							p.randompool.Randomize();
						} else {
							p.randompool.Clear();
						}
					}
					p.shared.hang.Cancel();
					break;
				case Model::EncoderAlts::PlayMode:
					if (ysb.has_value()) {
						p.data.settings.IncPlayMode(i, inc);
						p.player.RandomizeSteps(i);
					} else {
						p.data.settings.IncPlayMode(inc);
						p.player.RandomizeSteps();
					}
					p.shared.hang.Cancel();
					break;
				case Model::EncoderAlts::StartOffset:
					inc = hang.has_value() ? inc : 0;
					if (ysb.has_value()) {
						p.data.settings.IncStartOffset(i, inc);
					} else {
						p.data.settings.IncStartOffset(inc);
					}
					p.shared.hang.Set(encoder);
					break;
				case Model::EncoderAlts::PhaseOffset:
					inc = hang.has_value() ? inc : 0;
					if (ysb.has_value()) {
						p.data.settings.IncPhaseOffset(i, inc);
					} else {
						p.data.settings.IncPhaseOffset(inc);
					}
					p.shared.hang.Set(encoder);
					break;
				case Model::EncoderAlts::SeqLength:
					inc = hang.has_value() ? inc : 0;
					if (ysb.has_value()) {
						p.data.settings.IncLength(i, inc);
					} else {
						p.data.settings.IncLength(inc);
					}
					p.shared.hang.Set(encoder);
					break;
				case Model::EncoderAlts::Range:
					if (ysb.has_value()) {
						inc = hang.has_value() ? inc : 0;
						p.data.settings.IncRange(i, inc);
						p.shared.hang.Set(encoder);
					}
					break;
				case Model::EncoderAlts::ClockDiv:
					if (ysb.has_value()) {
						inc = hang.has_value() ? inc : 0;
						p.data.settings.IncClockDiv(i, inc);
						p.shared.hang.Set(encoder);
					} else {
						if (p.shared.internalclock.IsInternal()) {
							p.shared.data.bpm.Inc(inc, c.button.fine.is_high());
							p.shared.hang.Cancel();
						} else {
							inc = hang.has_value() ? inc : 0;
							p.shared.hang.Set(encoder);
							p.shared.data.clockdiv.Inc(inc);
						}
					}
					break;
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check();

		auto clockdiv = p.shared.data.clockdiv;

		auto length = std::make_optional(p.data.settings.GetLength());
		auto phaseoffset = std::make_optional(p.data.settings.GetPhaseOffset());
		auto startoffset = std::make_optional(p.data.settings.GetStartOffset());
		auto playmode = std::make_optional(p.data.settings.GetPlayMode());
		auto tpose = std::make_optional(p.data.settings.GetTranspose());
		auto range = Channel::Range{};
		auto random = p.randompool.IsRandomized() ? 1.f : 0.f;

		auto ysb = YoungestSceneButton();
		if (ysb.has_value()) {
			const auto chan = ysb.value();
			length = p.data.settings.GetLength(chan);
			phaseoffset = p.data.settings.GetPhaseOffset(chan);
			startoffset = p.data.settings.GetStartOffset(chan);
			playmode = p.data.settings.GetPlayMode(chan);
			clockdiv = p.data.settings.GetClockDiv(chan);
			tpose = p.data.settings.GetTranspose(chan);
			random = p.data.settings.GetRandomAmount(chan).Read();
			range = p.data.settings.GetRange(chan);
		}

		using namespace Model;
		namespace Setting = Palette::Setting;

		if (hang.has_value()) {
			switch (hang.value()) {
				case EncoderAlts::StartOffset: {
					const auto l = startoffset.value_or(p.data.settings.GetStartOffset());
					const auto col = startoffset.has_value() ? Setting::active : Setting::null;
					c.SetEncoderLed(l % SeqStepsPerPage, col);
					c.SetButtonLed(l / SeqStepsPerPage, true);
					break;
				}
				case EncoderAlts::SeqLength: {
					const auto l = length.value_or(p.data.settings.GetLength());
					const auto col = length.has_value() ? Setting::active : Setting::null;
					auto led = l % SeqStepsPerPage;
					SetEncoderLedsCount(led == 0 ? SeqStepsPerPage : led, 0, col);
					led = (l - 1) / SeqStepsPerPage;
					SetButtonLedsCount(led + 1, true);
					break;
				}
				case EncoderAlts::ClockDiv: {
					SetEncoderLedsAddition(clockdiv.Read(), Setting::clockdiv);
					break;
				}
				case EncoderAlts::PhaseOffset: {
					const auto o = p.player.GetFirstStep(ysb);
					const auto col = phaseoffset.has_value() ? Setting::active : Setting::null;
					c.SetEncoderLed(o % SeqStepsPerPage, col);
					c.SetButtonLed((o / SeqStepsPerPage) % SeqPages, true);
					break;
				}
				case EncoderAlts::Range: {
					DisplayRange(range);
					break;
				}
			}
		} else {
			auto col = startoffset.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(EncoderAlts::StartOffset, col);

			col = tpose.has_value() ? Palette::off.blend(Setting::transpose, tpose.value() / 12.f) : Setting::null;
			c.SetEncoderLed(EncoderAlts::Transpose, col);

			if (playmode.has_value()) {
				PlayModeLedAnnimation(playmode.value(), time_now);
			} else {
				c.SetEncoderLed(EncoderAlts::PlayMode, Setting::null);
			}

			col = length.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(EncoderAlts::SeqLength, col);

			col = phaseoffset.has_value() ? Setting::active : Setting::null;
			c.SetEncoderLed(EncoderAlts::PhaseOffset, col);

			if (p.shared.internalclock.IsInternal() && !ysb.has_value()) {
				if (p.shared.internalclock.Peek()) {
					col = Setting::bpm;
				} else {
					col = Palette::off;
				}
			} else {
				col = Setting::active;
			}
			c.SetEncoderLed(EncoderAlts::ClockDiv, col);

			if (!p.randompool.IsRandomized() || random == 0.f) {
				col = Palette::Random::none;
			} else {
				col = Palette::Random::none.blend(Palette::Random::color(p.randompool.GetSeed()), random);
			}

			c.SetEncoderLed(EncoderAlts::Random, col);

			c.SetEncoderLed(EncoderAlts::Range, ysb.has_value() ? Setting::active : Palette::off);
		}
	}

private:
	void PlayModeLedAnnimation(Catalyst2::Sequencer::Settings::PlayMode::Mode pm, uint32_t time_now) {
		using namespace Palette::Setting;
		static constexpr auto animation_duration = static_cast<float>(Clock::MsToTicks(1000));
		auto phase = (time_now / animation_duration);
		phase -= static_cast<uint32_t>(phase);
		Color col;

		using enum Catalyst2::Sequencer::Settings::PlayMode::Mode;

		if (pm == Forward) {
			col = playmode_fwd.blend(Palette::off, 1.f - phase);
		} else if (pm == Backward) {
			col = playmode_bck.blend(Palette::off, phase);
		} else if (pm == PingPong) {
			if (phase < .5f) {
				phase *= 2.f;
				col = playmode_bck.blend(Palette::off, phase);
			} else {
				phase -= .5f;
				phase *= 2.f;
				col = playmode_fwd.blend(Palette::off, 1.f - phase);
			}
		} else {
			col = Palette::Random::color(time_now >> 8);
		}
		c.SetEncoderLed(Model::EncoderAlts::PlayMode, col);
	}
};

} // namespace Catalyst2::Sequencer::Ui
