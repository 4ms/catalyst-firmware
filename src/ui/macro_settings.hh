#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Settings : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc([this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		if (!c.button.shift.is_high()) {
			return;
		}
		if (c.button.add.is_high()) {
			return;
		}
		if (c.button.morph.is_high()) {
			return;
		}
		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());
		const auto is_scene = YoungestSceneButton().has_value();

		switch (encoder) {
			case Model::MacroEncoderAlts::Random:
				if (is_scene) {
					for (auto [i, b] : countzip(c.button.scene)) {
						if (b.is_high()) {
							p.bank.IncRandomAmount(i, inc);
						}
					}
				} else {
					if (c.button.fine.is_high()) {
						if (inc > 0) {
							p.bank.randompool.Randomize();
						} else {
							p.bank.randompool.Clear();
						}
					}
				}
				p.shared.hang.Cancel();
				break;
			case Model::MacroEncoderAlts::ClockDiv:
				if (is_scene) {
					break;
				}
				inc = hang.has_value() ? inc : 0;
				p.IncClockDiv(inc);
				p.shared.hang.Set(encoder, p.shared.internalclock.TimeNow());
				break;
			case Model::MacroEncoderAlts::SliderSlew:
				if (is_scene) {
					break;
				}
				inc = hang.has_value() ? inc : 0;
				p.slider_slew.Inc(inc);
				p.shared.hang.Set(encoder, p.shared.internalclock.TimeNow());
				break;
			case Model::MacroEncoderAlts::SliderSlewCurve:
				if (is_scene) {
					break;
				}
				if (inc > 0)
					p.slider_slew.SetCurve(SliderSlew::Curve::Linear);
				else if (inc < 0)
					p.slider_slew.SetCurve(SliderSlew::Curve::Expo);
				p.shared.hang.Cancel();
				break;
			default:
				break;
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		ClearEncoderLeds();

		auto ysb = YoungestSceneButton();
		const auto is_scene = ysb.has_value();
		const auto scene = ysb.value_or(0);

		if (is_scene) {
			const auto random = p.bank.GetRandomAmount(scene);
			auto col = Palette::off.blend(Palette::Random::set, random);
			c.SetEncoderLed(Model::MacroEncoderAlts::Random, col);
		} else {
			const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());
			if (hang.has_value()) {
				switch (hang.value()) {
					case Model::MacroEncoderAlts::ClockDiv: {
						SetLedsClockDiv(p.GetClockDiv().Read());
					} break;

					case Model::MacroEncoderAlts::SliderSlew: {
						float num_lights = p.slider_slew.Value() * 8.f;
						SetEncoderLedsFloat(num_lights, Palette::Setting::slider_slew, Palette::very_dim_grey);
					} break;
				}
			} else {
				c.SetEncoderLed(Model::MacroEncoderAlts::ClockDiv, Palette::Setting::active);
				c.SetEncoderLed(Model::MacroEncoderAlts::SliderSlew,
								Palette::very_dim_grey.blend(Palette::Setting::slider_slew, p.slider_slew.Value()));
				auto col = p.slider_slew.GetCurve() == SliderSlew::Curve::Linear ? Palette::Setting::curve_linear :
																				   Palette::Setting::curve_expo;
				c.SetEncoderLed(Model::MacroEncoderAlts::SliderSlewCurve, col);
				if (c.button.fine.is_high()) {
					col = p.bank.randompool.IsRandomized() ? Palette::Random::color(p.bank.randompool.GetSeed()) :
															 Palette::Setting::null;
					c.SetEncoderLed(Model::MacroEncoderAlts::Random, col);
				}
			}
		}
	}
};

} // namespace Catalyst2::Macro::Ui
