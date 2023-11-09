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
		if (!c.button.shift.is_high())
			return;

		if (c.button.add.is_high())
			return;

		interface = this;
	}
	void OnEncoderInc(uint8_t encoder, int32_t inc) override {
		const auto time_now = HAL_GetTick();
		const auto hang = p.shared.hang.Check(time_now);
		const auto is_scene = c.YoungestSceneButton().has_value();

		switch (encoder) {
			case Model::EncoderAlts::Random:
				if (is_scene) {
					for (auto [i, b] : countzip(c.button.scene)) {
						if (b.is_high())
							p.bank.IncRandomAmount(i, inc);
					}
				} else {
					if (inc > 0)
						p.shared.randompool.RandomizeScene();
					else
						p.shared.randompool.ClearScene();
				}
				p.shared.hang.Cancel();
				break;
			case Model::EncoderAlts::ClockDiv:
				if (is_scene)
					break;
				inc = hang.has_value() ? inc : 0;
				p.shared.clockdiv.Inc(inc);
				p.shared.hang.Set(encoder, time_now);
				break;
				// case Model::EncoderAlts::Transpose:
				//	if ()
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override {
		c.ClearButtonLeds();
		c.ClearEncoderLeds();

		const auto is_scene = c.YoungestSceneButton().has_value();
		const auto scene = c.YoungestSceneButton().value_or(0);

		if (is_scene) {
			const auto random = p.bank.GetRandomAmount(scene);
			auto col = Palette::red;
			if (p.shared.randompool.IsRandomized() && random > 0.f)
				col = Palette::off.blend(Palette::from_raw(p.shared.randompool.GetSeed()), random);
			c.SetEncoderLed(Model::EncoderAlts::Random, col);
		} else {
			const auto time_now = HAL_GetTick();
			const auto hang = p.shared.hang.Check(time_now);
			if (hang.has_value()) {
				if (hang.value() == Model::EncoderAlts::ClockDiv) {
					c.SetEncoderLedsAddition(p.shared.clockdiv.Read(), Palette::blue);
				}
			} else {
				c.SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::seqhead);
				const auto col = p.shared.randompool.IsRandomized() ? Palette::from_raw(p.shared.randompool.GetSeed()) :
																	  Palette::globalsetting;
				c.SetEncoderLed(Model::EncoderAlts::Random, col);
			}
		}
	}
};

} // namespace Catalyst2::Macro::Ui
