#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Add : public Usual {
	bool first;

public:
	using Usual::Usual;
	void Init() override {
		first = true;
		c.button.shift.clear_events();
	}
	void Update(Abstract *&interface) override {
		if (c.button.shift.just_went_high()) {
			if (p.pathway.OnAScene() || p.pathway.SceneLeft() == p.pathway.SceneRight())
				p.pathway.RemoveSceneNearest();
		}
		if (!c.button.add.is_high() && !c.button.shift.is_high())
			return;

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t button) override {
		auto &path = p.pathway;

		if (c.button.shift.is_high()) {
			if (path.OnAScene()) {
				if (path.SceneNearest() == button)
					path.RemoveSceneNearest();
			} else {
				if (path.SceneLeft() == button)
					path.RemoveSceneLeft();
				else if (path.SceneRight() == button)
					path.RemoveSceneRight();
			}
			return;
		}

		if (!first) {
			path.InsertScene(button, true);
			return;
		}

		first = false;

		if (path.OnAScene())
			path.ReplaceScene(button);
		else
			path.InsertScene(button, false);
	}
	void PaintLeds(const Model::OutputBuffer &outs) override {
		ClearButtonLeds();

		auto count = p.pathway.size();
		const auto phase = 1.f / (Pathway::MaxPoints / static_cast<float>(count));

		while (count > 8)
			count -= 8;

		SetEncoderLedsCount(count, 0, Palette::green.blend(Palette::red, phase));

		if (p.pathway.OnAScene()) {
			c.SetButtonLed(p.pathway.SceneNearest(), true);
		} else {
			c.SetButtonLed(p.pathway.SceneLeft(), true);
			c.SetButtonLed(p.pathway.SceneRight(), true);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
