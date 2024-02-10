#pragma once

#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class PageParams : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		if (!p.IsChannelSelected()) {
			p.SelectChannel(0);
		}
	}
	void Update(Abstract *&interface) override {
		if (!c.button.shift.is_high() || !p.shared.youngest_scene_button.has_value())
			return; // exit PageParams mode

		if (!p.IsChannelSelected())
			return;

		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) {
			for (auto [page, b] : countzip(c.button.scene)) {
				if (b.is_high()) {
					OnEncoderInc(encoder, inc, page);
				}
			}
		});

		// Stay in PageParams mode
		interface = this;
	}

	void OnEncoderInc(uint8_t encoder, int32_t inc, uint32_t page) {
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
				p.shared.hang.Cancel();
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
				p.shared.hang.Cancel();
			} break;

			case Model::SeqEncoderAlts::PlayMode:
				// inc > 0 => reverse
				// inc < 0 => randomize order of steps
				p.shared.hang.Cancel();
				break;

			case Model::SeqEncoderAlts::PhaseOffset: {
				if (inc > 0)
					p.RotateStepsRight(page_start, page_start + Model::SeqStepsPerPage);
				else if (inc < 0)
					p.RotateStepsLeft(page_start, page_start + Model::SeqStepsPerPage);

				// TODO: some display to show the rotation?
				// p.shared.hang.Set(encoder, time_now);
				p.shared.hang.Cancel();
			} break;
		}
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		c.SetButtonLed(p.shared.youngest_scene_button.value(), true);

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		using namespace Model;
		namespace Setting = Palette::Setting;

		// if (hang.has_value()) {
		// } else {
		// TODO: what do we display? Maybe the hang display could be the steps as we change them?
		c.SetEncoderLed(SeqEncoderAlts::Transpose, Setting::active);
		c.SetEncoderLed(SeqEncoderAlts::Random, Setting::active);
		c.SetEncoderLed(SeqEncoderAlts::PhaseOffset, Setting::active);
		c.SetEncoderLed(SeqEncoderAlts::PlayMode, Setting::active);
		// }
	}
};
} // namespace Catalyst2::Ui::Sequencer
