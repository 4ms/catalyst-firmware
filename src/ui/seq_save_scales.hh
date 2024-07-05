#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "sequencer.hh"

namespace Catalyst2::Ui::Sequencer
{
class SaveScales : public Usual {
	uint8_t did_save = Model::NumChans;

public:
	using Usual::Usual;
	void Init() override {
		did_save = Model::NumChans;
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });
		if (!c.button.bank.is_high() || !c.button.add.is_high()) {
			SwitchUiMode(main_ui);
			if (did_save < Model::NumChans) {
				p.shared.data.custom_scale[did_save] = p.toScale();
				p.UpdateChannelMode();
				p.shared.do_save_shared = true;
			}
			return;
		}
		if (c.button.shift.is_high()) {
			SwitchUiMode(main_ui);
			return;
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) {
		if (dir < 0) {
			did_save = Model::NumChans;
		} else if (did_save == Model::NumChans) {
			did_save = encoder;
		}
	}
	void OnSceneButtonRelease(uint8_t page) {
		if (did_save < Model::NumChans) {
			return;
		}
		p.SelectChannel(page);
		if (!p.IsChannelSelected()) {
			p.SelectChannel(page);
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		ClearEncoderLeds(c);
		c.SetPlayLed(!p.IsPaused());
		c.SetButtonLed(p.GetSelectedChannel(), true);
		const auto &col = Palette::Scales::color;
		if (did_save == Model::NumChans) {
			const auto blink = Controls::TimeNow() & (1u << 9);
			if (blink) {
				for (auto i = 0u; i < Model::NumChans; i++) {
					c.SetEncoderLed(i, col[col.size() - Model::num_custom_scales + i - 1]);
				}
			}
		} else {
			c.SetEncoderLed(did_save, col[col.size() - Model::num_custom_scales + did_save - 1]);
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
