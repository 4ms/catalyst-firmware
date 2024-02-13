#pragma once

#include "conf/flash_layout.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"

namespace Catalyst2::Ui::Macro
{

class Add : public Usual {
	bool first_insert;

public:
	using Usual::Usual;
	void Init() override {
		first_insert = true;
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		const auto add = c.button.add.is_high();
		const auto shift = c.button.shift.is_high();

		if (!add && !shift) {
			return;
		}

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t button) {
		auto &path = p.bank.pathway;

		if (c.button.shift.is_high()) {
			DeleteScene(button);
			first_insert = false;
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
		auto &path = p.bank.pathway;
		if (path.OnAScene()) {
			if (path.SceneRelative() == button) {
				path.RemoveSceneRelative();
			}
		} else {
			if (path.SceneRelative(-1) == button) {
				path.RemoveSceneRelative(-1);
			} else if (path.SceneRelative(1) == button) {
				path.RemoveSceneRelative(1);
			}
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);

		auto count = p.bank.pathway.size();
		const auto phase = 1.f / (Catalyst2::Macro::Pathway::Normal::MaxPoints / static_cast<float>(count));

		while (count > 8) {
			count -= 8;
		}
		SetEncoderLedsCount(c, count, 0, Palette::Pathway::color(phase));

		if (p.bank.pathway.OnAScene()) {
			c.SetButtonLed(p.bank.pathway.SceneRelative(), true);
		} else {
			c.SetButtonLed(p.bank.pathway.SceneRelative(-1), true);
			c.SetButtonLed(p.bank.pathway.SceneRelative(1), true);
		}
	}
};
} // namespace Catalyst2::Ui::Macro
