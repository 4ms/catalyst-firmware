#pragma once

#include "abstract.hh"
#include "params.hh"

namespace Catalyst2::Ui::Sequencer
{
inline void PlayModeLedAnimation(Controls &c, Catalyst2::Sequencer::Settings::PlayMode::Mode pm, uint32_t time_now) {
	using namespace Palette::Setting;
	static constexpr auto animation_duration = static_cast<float>(Clock::MsToTicks(1000));
	auto phase = (time_now / animation_duration);
	phase -= static_cast<uint32_t>(phase);
	Color col;

	using enum Catalyst2::Sequencer::Settings::PlayMode::Mode;

	if (pm == Forward) {
		col = playmode_fwd.blend(Palette::off, 1.f - phase);
	} else if (pm == Backward) {
		col = playmode_bck.blend(Palette::off, phase);
	} else if (pm == PingPong) {
		if (phase < .5f) {
			phase *= 2.f;
			col = playmode_bck.blend(Palette::off, phase);
		} else {
			phase -= .5f;
			phase *= 2.f;
			col = playmode_fwd.blend(Palette::off, 1.f - phase);
		}
	} else {
		col = Palette::Random::color(time_now >> 8);
	}
	c.SetEncoderLed(Model::Sequencer::EncoderAlts::PlayMode, col);
}

class Usual : public Abstract {

	uint8_t last_playhead_pos = Model::Sequencer::NumPages;

public:
	Catalyst2::Sequencer::Interface &p;
	Usual(Catalyst2::Sequencer::Interface &p, Controls &c)
		: Abstract{c}
		, p{p} {
	}
	void Common() final {
		p.seqclock.Update();

		if (c.jack.reset.just_went_high()) {
			p.Reset(false);
		}

		if (c.jack.trig.just_went_high()) {
			p.Trig();
		}

		const auto phase = (c.ReadSlider() + c.ReadCv()) / 4095.f;
		p.Update(phase);
	}

	void PaintStepValues(uint8_t page, uint8_t chan) {
		const auto step_offset = Catalyst2::Sequencer::SeqPageToStep(page);
		const auto is_cv = !p.slot.settings.GetChannelMode(chan).IsGate();
		const auto fine_pressed = c.button.fine.is_high();
		const auto range = p.slot.settings.GetRange(chan);
		const auto playhead_page = p.player.GetPlayheadPage(chan);
		const auto playhead_pos = p.player.GetPlayheadStepOnPage(chan);

		for (auto i = 0u; i < Model::Sequencer::Steps::PerPage; i++) {
			const auto step = p.GetStep(step_offset + i);
			auto color = is_cv		  ? Palette::Cv::fromLevel(step.ReadCv(), range) :
						 fine_pressed ? Palette::Gate::fromTrigDelay(step.ReadTrigDelay()) :
										Palette::Gate::fromLevelSequencer(step.ReadGate());

			if (page == playhead_page && i == playhead_pos)
				SetPlayheadPageStepLed(i, color);
			else
				c.SetEncoderLed(i, color);
		}
	}

protected:
	void BlinkSelectedPage(uint8_t page) {
		c.SetButtonLed(page, ((p.shared.internalclock.TimeNow() >> 8) & 1) > 0);
	}
	void SetPlayheadPageStepLed(uint8_t playhead_pos, Color base_color) {
		if (last_playhead_pos != playhead_pos) {
			last_playhead_pos = playhead_pos;
			p.seqclock.ResetPeek();
		}

		c.SetEncoderLed(
			playhead_pos,
			base_color.blend(Palette::SeqHead::color, std::clamp(1.f - 2.f * p.seqclock.PeekPhase(), 0.f, 1.f)));
	}
};
} // namespace Catalyst2::Ui::Sequencer
