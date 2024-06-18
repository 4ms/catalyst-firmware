#pragma once

#include "abstract.hh"
#include "clock.hh"
#include "params.hh"

namespace Catalyst2::Ui::Sequencer
{
inline void PlayModeLedAnimation(Controls &c, Catalyst2::Sequencer::Settings::PlayMode::Mode pm) {
	using namespace Palette::Setting;
	static constexpr auto animation_duration = static_cast<float>(Clock::MsToTicks(1000));
	const auto time_now = Controls::TimeNow();
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

public:
	Catalyst2::Sequencer::Interface &p;
	Abstract &main_ui;

	Usual(Catalyst2::Sequencer::Interface &p, Controls &c, Abstract *main_ui)
		: Abstract{c}
		, p{p}
		, main_ui{*main_ui} {
	}
	void Common() final {
		p.seqclock.external = c.sense.trig.is_high();

		if (c.jack.reset.just_went_high()) {
			if (p.slot.clock_sync_mode.mode == Clock::Sync::Mode::SYNCED) {
				p.Reset();
			} else {
				p.Play();
			}
		} else if (c.jack.reset.just_went_low()) {
			if (p.slot.clock_sync_mode.mode == Clock::Sync::Mode::DIN_SYNC) {
				p.Stop(true);
			}
		}

		if (c.jack.trig.just_went_high()) {
			p.Trig();
		}

		const auto phase = (c.ReadSlider() + c.ReadCv()) / 4096.f;
		p.Update(phase);
	}

protected:
	void PaintStepValues(uint8_t page) {
		const auto chan = p.GetSelectedChannel();
		const auto step_offset = Catalyst2::Sequencer::SeqPageToStep(page);
		const auto is_cv = !p.slot.settings.GetChannelMode(chan).IsGate();
		const auto fine_pressed = c.button.fine.is_high() && !c.button.morph.is_high();
		const auto seqhead_col =
			p.slot.settings.GetChannelMode(chan).IsMuted() ? Palette::SeqHead::mute : Palette::SeqHead::active;

		if (is_cv) {
			const auto range = p.slot.settings.GetRange(chan);
			for (auto step_i = 0u; step_i < Model::Sequencer::Steps::PerPage; step_i++) {
				const auto step = p.GetStep(step_offset + step_i);
				const auto color = Palette::Cv::fromLevel(step.ReadCv(), range);
				PaintStep(page, step_i, color, seqhead_col);
			}
		} else {
			if (!fine_pressed) {
				for (auto step_i = 0u; step_i < Model::Sequencer::Steps::PerPage; step_i++) {
					const auto step = p.GetStep(step_offset + step_i);
					const auto color = Palette::Gate::fromLevelSequencer(step.ReadGate());
					PaintStep(page, step_i, color, seqhead_col);
				}
			} else {
				for (auto step_i = 0u; step_i < Model::Sequencer::Steps::PerPage; step_i++) {
					const auto step = p.GetStep(step_offset + step_i);
					const auto color = Palette::Gate::fromTrigDelay(step.ReadTrigDelay());
					PaintStep(page, step_i, color, seqhead_col);
				}
			}
		}
	}
	void PaintStep(uint8_t page, uint8_t step, Color base_color, Color seq_head) {
		const auto playhead_page = p.GetPlayheadPage();
		const auto playhead_pos = p.GetPlayheadStepOnPage();
		if (page == playhead_page && step == playhead_pos)
			SetPlayheadStepLed(step, base_color, seq_head);
		else
			c.SetEncoderLed(step, base_color);
	}
	void SetPlayheadStepLed(uint8_t playhead_pos, Color base_color, Color seq_head) {
		const auto color = p.ShowPlayhead() ?
							   base_color.blend(seq_head, std::clamp(1.f - 2.f * p.seqclock.PeekPhase(), 0.f, 1.f)) :
							   base_color;
		c.SetEncoderLed(playhead_pos, color);
	}
	void BlinkSelectedPage(uint8_t page) {
		c.SetButtonLed(page, ((Controls::TimeNow() >> 8) & 1) > 0);
	}
};
} // namespace Catalyst2::Ui::Sequencer
