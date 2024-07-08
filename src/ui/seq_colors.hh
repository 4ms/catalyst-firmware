#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "sequencer.hh"
#include <algorithm>

// maybe we come up with a set of values like 7 different notes in different octaves, three negative, one zero, and
// three positive

namespace Catalyst2::Ui::Sequencer
{
class Colors : public Usual {

public:
	using Usual::Usual;
	void Init() override {
	}
	void Update() override {
		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });
		if (!c.button.bank.is_high()) {
			SwitchUiMode(main_ui);
			p.shared.do_save_shared = true;
			return;
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) {
		if (encoder > 0) {
			return;
		}
		p.shared.data.palette[0] =
			std::clamp<int32_t>(p.shared.data.palette[0] + dir, 0, Palette::Cv::num_palettes - 1);
		std::fill(p.shared.data.palette.begin() + 1, p.shared.data.palette.end(), p.shared.data.palette[0]);
	}
	void OnSceneButtonRelease(uint8_t page) {
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		const auto blink = Controls::TimeNow() & (1u << 9);
		if (blink) {
			c.SetEncoderLed(0, Palette::dim_grey);
		}
		using namespace Channel::Output;
		constexpr std::array notes = {from_octave_note(-4, 0),
									  from_octave_note(-2, 1),
									  from_octave_note(-1, 2),
									  from_octave_note(0, 0),
									  from_octave_note(1, 3),
									  from_octave_note(4, 4),
									  from_octave_note(7, 5)};
		for (auto i = 0u; i < 7; i++) {
			c.SetEncoderLed(i + 1, Palette::Cv::fromOutput(p.shared.data.palette[0], notes[i]));
		}
	}
};

} // namespace Catalyst2::Ui::Sequencer
