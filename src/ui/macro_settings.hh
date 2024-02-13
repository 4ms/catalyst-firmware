#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Ui::Macro
{

class Settings : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
	}
	void Update(Abstract *&interface) override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

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
		const auto is_scene = p.shared.youngest_scene_button.has_value();

		switch (encoder) {
			using namespace Model::Macro;
			case EncoderAlts::Random:
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
			case EncoderAlts::OutputOverride:
				if (is_scene) {
					break;
				}
				p.IncOutputOverride(inc);
				p.shared.hang.Cancel();
				break;
			case EncoderAlts::ClockDiv:
				if (is_scene) {
					break;
				}
				inc = hang.has_value() ? inc : 0;
				p.IncClockDiv(inc);
				p.shared.hang.Set(encoder, p.shared.internalclock.TimeNow());
				break;
			case EncoderAlts::SliderSlew:
				if (is_scene) {
					break;
				}
				inc = hang.has_value() ? inc : 0;
				p.slider_slew.Inc(inc);
				p.shared.hang.Set(encoder, p.shared.internalclock.TimeNow());
				break;
			case EncoderAlts::SliderSlewCurve:
				if (is_scene) {
					break;
				}
				if (inc > 0)
					p.slider_slew.SetCurve(Catalyst2::Macro::SliderSlew::Curve::Linear);
				else if (inc < 0)
					p.slider_slew.SetCurve(Catalyst2::Macro::SliderSlew::Curve::Expo);
				p.shared.hang.Cancel();
				break;
			default:
				break;
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);

		auto ysb = p.shared.youngest_scene_button;
		const auto is_scene = ysb.has_value();
		const auto scene = ysb.value_or(0);

		using namespace Model::Macro;

		if (is_scene) {
			const auto random = p.bank.GetRandomAmount(scene);
			auto col = Palette::off.blend(Palette::Random::set, random);
			c.SetEncoderLed(EncoderAlts::Random, col);
		} else {
			const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());
			if (hang.has_value()) {
				switch (hang.value()) {
					case EncoderAlts::ClockDiv: {
						SetLedsClockDiv(c, p.GetClockDiv().Read());
					} break;

					case EncoderAlts::SliderSlew: {
						float num_lights = p.slider_slew.Value() * 8.f;
						SetEncoderLedsFloat(c, num_lights, Palette::Setting::slider_slew, Palette::very_dim_grey);
					} break;
				}
			} else {
				c.SetEncoderLed(EncoderAlts::OutputOverride,
								p.GetOutputOverride() ? Palette::Setting::OutputOverride::on :
														Palette::Setting::OutputOverride::off);
				c.SetEncoderLed(EncoderAlts::ClockDiv, Palette::Setting::active);
				c.SetEncoderLed(EncoderAlts::SliderSlew,
								Palette::very_dim_grey.blend(Palette::Setting::slider_slew, p.slider_slew.Value()));
				auto col = p.slider_slew.GetCurve() == Catalyst2::Macro::SliderSlew::Curve::Linear ?
							   Palette::Setting::curve_linear :
							   Palette::Setting::curve_expo;
				c.SetEncoderLed(EncoderAlts::SliderSlewCurve, col);
				if (c.button.fine.is_high()) {
					col = p.bank.randompool.IsRandomized() ? Palette::Random::color(p.bank.randompool.GetSeed()) :
															 Palette::Setting::null;
					c.SetEncoderLed(EncoderAlts::Random, col);
				}
			}
		}
	}
};

} // namespace Catalyst2::Ui::Macro
