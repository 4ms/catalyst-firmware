#pragma once

#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class PageParams : public Usual {
	uint8_t page{};

public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		if (!p.IsChannelSelected()) {
			p.SelectChannel(0);
		}
		// TODO: do we need this
		//  p.shared.modeswitcher.SetAlarm(p.shared.internalclock.TimeNow());
	}
	void Update(Abstract *&interface) override {
		if (!c.button.shift.is_high() || !p.shared.youngest_scene_button.has_value())
			return; // exit PageParams mode

		if (!p.IsChannelSelected())
			return;

		// TODO: handle multiple page buttons, apply to all of them
		page = p.shared.youngest_scene_button.value();

		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) { OnEncoderInc(encoder, inc); });

		// Stay in PageParams mode
		interface = this;
	}

	void OnEncoderInc(uint8_t encoder, int32_t inc) {
		const auto hang = p.shared.hang.Check(p.shared.internalclock.TimeNow());

		const auto chan = p.GetSelectedChannel();
		const auto time_now = p.shared.internalclock.TimeNow();
		const auto page_start = page * Model::SeqStepsPerPage;

		switch (encoder) {
			case Model::SeqEncoderAlts::Transpose: {
				const auto fine = c.button.fine.is_high();
				const auto page_start = page * Model::SeqStepsPerPage;
				for (auto step = 0u; step < Model::SeqStepsPerPage; step++) {
					p.IncStepInSequence(step + page_start, inc, fine);
				}
				p.shared.hang.Set(encoder, time_now);
			} break;

			case Model::SeqEncoderAlts::Random: {

				auto random_range = p.slot.settings.GetRandomOrGlobal(p.GetSelectedChannel());
				if (random_range == 0)
					random_range = Random::Amount::inc;

				random_range *= Model::output_octave_range * 12;
				// TODO: create a buffer of random values upon entry and the encoder selects one
				for (auto step = 0u; step < Model::SeqStepsPerPage; step++) {
					float random_inc = int8_t(std::rand() / 256) / float(-INT8_MIN);
					p.IncStepInSequence(step + page_start, random_range * random_inc, false);
				}
				p.shared.hang.Set(encoder, time_now);
			} break;

			case Model::SeqEncoderAlts::PlayMode:
				if (inc > 0)
					p.ReverseStepOrder(page_start, page_start + Model::SeqStepsPerPage - 1);
				else if (inc < 0)
					p.RandomShuffleStepOrder(page_start, page_start + Model::SeqStepsPerPage - 1);
				p.shared.hang.Set(encoder, time_now);
				break;

			case Model::SeqEncoderAlts::PhaseOffset: {
				if (inc > 0)
					p.RotateStepsRight(page_start, page_start + Model::SeqStepsPerPage - 1);
				else if (inc < 0)
					p.RotateStepsLeft(page_start, page_start + Model::SeqStepsPerPage - 1);

				p.shared.hang.Set(encoder, time_now);
			} break;
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		c.SetButtonLed(page, true);

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		using namespace Model;
		namespace Setting = Palette::Setting;

		if (hang.has_value()) {
			const auto chan = p.GetSelectedChannel();
			const auto is_gate = p.slot.settings.GetChannelMode(chan).IsGate();
			const auto pvals = is_gate ? p.GetPageValuesGate(page) : p.GetPageValuesCv(page);
			auto display_func = is_gate ? [](Model::Output::type v) { return Palette::GateBlend(v); } :
										  [](Model::Output::type v) { return Palette::CvBlend(v); };
			for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
				c.SetEncoderLed(i, display_func(pvals[i]));
			}

		} else {
			c.SetEncoderLed(SeqEncoderAlts::Transpose, Setting::active);
			c.SetEncoderLed(SeqEncoderAlts::Random, Setting::active);
			c.SetEncoderLed(SeqEncoderAlts::PhaseOffset, Setting::active);
			c.SetEncoderLed(SeqEncoderAlts::PlayMode, Setting::active);
		}
	}
};
} // namespace Catalyst2::Ui::Sequencer
