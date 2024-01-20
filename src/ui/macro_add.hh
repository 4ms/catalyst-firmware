#pragma once

#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Macro::Ui
{

class Add : public Usual {
	bool first_insert;

public:
	using Usual::Usual;
	void Init() override {
		first_insert = true;
		if (c.button.shift.is_high()) {
			p.shared.reset.Notify(true);
		}
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonReleased([this](uint8_t button) { OnSceneButtonRelease(button); });

		const auto add = c.button.add.is_high();
		const auto shift = c.button.shift.is_high();
		// TODO: Cleanup the logic for reset, too many negatives is confusing
		// TODO: If the interface needs to change (!add && !shift) then call ChangeMode() or something. Return nothing
		// vs. setting interface = this and then returning is very subtle, hard to follow Also the reset.Check() seems
		// unrelated... or is it?
		if (!add || !shift) {
			p.shared.reset.Notify(false);
		}
		if ((!add && !shift) || p.shared.reset.Check()) {
			return;
		}
		interface = this;
	}
	void OnSceneButtonRelease(uint8_t button) {
		auto &path = p.pathway;
		p.shared.reset.Notify(false);

		if (c.button.shift.is_high()) {
			DeleteScene(button);
			return;
		}

		if (first_insert) {
			first_insert = false;

			if (path.OnAScene()) {
				path.ReplaceScene(button);
			} else {
				path.InsertScene(button);
			}
		} else {
			path.InsertSceneAfterLast(button);
		}
	}
	void DeleteScene(uint8_t button) {
		auto &path = p.pathway;
		if (path.OnAScene()) {
			if (path.SceneNearest() == button) {
				path.RemoveSceneNearest();
			}
		} else {
			if (path.SceneLeft() == button) {
				path.RemoveSceneLeft();
			} else if (path.SceneRight() == button) {
				path.RemoveSceneRight();
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();

		auto count = p.pathway.size();
		const auto phase = 1.f / (Pathway::MaxPoints / static_cast<float>(count));

		while (count > 8) {
			count -= 8;
		}
		SetEncoderLedsCount(count, 0, Palette::Pathway::color(phase));

		if (p.pathway.OnAScene()) {
			c.SetButtonLed(p.pathway.SceneNearest(), true);
		} else {
			c.SetButtonLed(p.pathway.SceneLeft(), true);
			c.SetButtonLed(p.pathway.SceneRight(), true);
		}
	}
};
} // namespace Catalyst2::Macro::Ui
