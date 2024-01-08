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
		const auto hang = p.shared.hang.Check();
		const auto is_scene = YoungestSceneButton().has_value();

		switch (encoder) {
			case Model::EncoderAlts::Random:
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
			case Model::EncoderAlts::ClockDiv:
				if (is_scene) {
					break;
				}
				inc = hang.has_value() ? inc : 0;
				p.shared.data.clockdiv.Inc(inc);
				p.shared.hang.Set(encoder);
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
			c.SetEncoderLed(Model::EncoderAlts::Random, col);
		} else {
			const auto hang = p.shared.hang.Check();
			if (hang.has_value()) {
				if (hang.value() == Model::EncoderAlts::ClockDiv) {
					SetEncoderLedsAddition(p.shared.data.clockdiv.Read(), Palette::Setting::active);
				}
			} else {
				c.SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::SeqHead::color);
				if (c.button.fine.is_high()) {
					const auto col = p.bank.randompool.IsRandomized() ?
										 Palette::Random::color(p.bank.randompool.GetSeed()) :
										 Palette::Setting::null;
					c.SetEncoderLed(Model::EncoderAlts::Random, col);
				}
			}
		}
	}
};

} // namespace Catalyst2::Macro::Ui
