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
		p.shared.save.SetAlarm(p.shared.internalclock.TimeNow());
		p.Reset(true);
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!p.shared.youngest_scene_button) {
			p.shared.save.SetAlarm(p.shared.internalclock.TimeNow());
		}

		if (p.shared.blinker.IsSet()) {
			ClearButtonLeds(c);
			return;
		}

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t scene) {
		if (!p.shared.save.Check(p.shared.internalclock.TimeNow())) {
			// load
			p.Load(scene);
			p.shared.blinker.Set(scene, 4, 125, p.shared.internalclock.TimeNow());
		} else {
			p.Save(scene);
			p.shared.do_save = true;
			p.shared.blinker.Set(scene, 16, 500, p.shared.internalclock.TimeNow());
		}
	}

	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		if (p.shared.youngest_scene_button.has_value()) {
			const auto time_now = p.shared.internalclock.TimeNow();
			if (p.shared.save.Check(time_now)) {
				SetButtonLedsCount(c, Model::Sequencer::NumSlots, true);
			} else {
				c.SetButtonLed(p.shared.youngest_scene_button.value(), true);
			}
		} else {
			SetButtonLedsCount(c, Model::Sequencer::NumSlots, true);
			if ((p.shared.internalclock.TimeNow() >> 10u) & 0x01) {
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
