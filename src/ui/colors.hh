#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "macro_common.hh"
#include "params.hh"
#include "seq_common.hh"
#include <algorithm>

namespace Catalyst2::Ui
{
template<typename Usual>
class Colors : public Usual {

public:
	using Usual::Usual;
	void Update() override {
		ForEachEncoderInc(Usual::c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });
		if (!Usual::c.button.bank.is_high()) {
			Usual::SwitchUiMode(Usual::main_ui);
			Usual::p.shared.do_save_shared = true;
			return;
		}
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) {
		if (encoder > 0) {
			return;
		}
		auto &s = Usual::p.shared.data.palette;
		s[0] = std::clamp<int32_t>(s[0] + dir, 0, Palette::Cv::num_palettes - 1);
		std::fill(s.begin() + 1, s.end(), s[0]);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(Usual::c);
		const auto blink = Controls::TimeNow() & (1u << 9);
		if (blink) {
			Usual::c.SetEncoderLed(0, Palette::dim_grey);
		}
		using namespace Channel::Output;
		constexpr std::array notes = {from_octave_note(-3, 6),
									  from_octave_note(-2, 8),
									  from_octave_note(-1, 10),
									  from_octave_note(0, 0),
									  from_octave_note(1, 2),
									  from_octave_note(2, 4),
									  from_octave_note(3, 5)};
		for (auto i = 0u; i < 7; i++) {
			Usual::c.SetEncoderLed(i + 1, Palette::Cv::fromOutput(Usual::p.shared.data.palette[0], notes[i]));
		}
	}
};

using SeqColors = Colors<Sequencer::Usual>;
using MacroColors = Colors<Macro::Usual>;

} // namespace Catalyst2::Ui
