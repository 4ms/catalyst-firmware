#pragma once

#include "clock.hh"
#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "ui/abstract.hh"

namespace Catalyst2::Ui::Sequencer
{
class Save : public Usual {

public:
	using Usual::Usual;
	void Init() override {
		p.shared.blinker.Cancel();
		p.shared.save.SetAlarm();
		c.button.morph.clear_events();
		c.button.bank.clear_events();
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!p.shared.youngest_scene_button) {
			p.shared.save.SetAlarm();
		}

		if (p.shared.blinker.IsSet()) {
			ClearButtonLeds(c);
			return;
		}

		if (c.button.morph.just_went_high() || c.button.bank.just_went_high() || c.button.fine.is_high() ||
			c.button.add.is_high() || c.button.shift.is_high())
		{
			return;
		}

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t scene) {
		if (!p.shared.save.Check()) {
			// load
			p.Load(scene);
			p.shared.blinker.Set(scene, 4, 125);
		} else {
			p.Save(scene);
			p.shared.do_save_seq = true;
			p.shared.blinker.Set(scene, 16, 500);
		}
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		if (p.shared.youngest_scene_button.has_value()) {
			if (p.shared.save.Check()) {
				SetButtonLedsCount(c, Model::Sequencer::NumSlots, true);
			} else {
				c.SetButtonLed(p.shared.youngest_scene_button.value(), true);
			}
		} else {
			SetButtonLedsCount(c, Model::Sequencer::NumSlots, true);
			if ((Controls::TimeNow() >> 10u) & 0x01) {
				for (auto i = 0u; i < Model::Sequencer::NumSlots; i++) {
					if (i == p.GetStartupSlot()) {
						continue;
					}
					c.SetButtonLed(i, false);
				}
			}
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
