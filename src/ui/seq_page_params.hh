#pragma once

#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Ui::Sequencer
{
class PageParams : public Usual {
	std::array<bool, Model::SeqPages> selected_pages{};
	bool is_latching_scene_buts = false;

public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		if (!p.IsChannelSelected()) {
			p.SelectChannel(0);
		}

		is_latching_scene_buts = false;
		for (auto [page, selected] : enumerate(selected_pages)) {
			selected = c.button.scene[page].is_pressed();
		}
	}
	void Update(Abstract *&interface) override {
		if (!c.button.shift.is_high() && !p.shared.youngest_scene_button.has_value())
			return; // exit PageParams mode

		auto num_pressed = 0u;
		ForEachSceneButtonPressed(c, [&, this](auto page) {
			if (!selected_pages[page])
				p.shared.hang.Set({}, p.shared.internalclock.TimeNow());

			selected_pages[page] = true;
			num_pressed++;
		});

		// When no scene buttons are down, latch the previously held buttons
		if (num_pressed == 0)
			is_latching_scene_buts = true;

		// The first time a button is pressed while latching, clear the latched buttons
		if (is_latching_scene_buts && num_pressed > 0) {
			is_latching_scene_buts = false;
			for (auto &is_selected : selected_pages)
				is_selected = false;
		}

		ForEachEncoderInc(c, [this](uint8_t encoder, int32_t inc) {
			for (auto [page, is_selected] : enumerate(selected_pages)) {
				if (is_selected)
					OnEncoderInc(encoder, inc, page);
			}
		});

		// Stay in PageParams mode
		interface = this;
	}

	void OnEncoderInc(uint8_t encoder, int32_t inc, uint32_t page) {
		const auto time_now = p.shared.internalclock.TimeNow();
		const auto page_start = page * Model::SeqStepsPerPage;
		p.shared.hang.Set(encoder, time_now);

		switch (encoder) {
			case Model::SeqEncoderAlts::Transpose: {
				const auto fine = c.button.fine.is_high();
				const auto page_start = page * Model::SeqStepsPerPage;
				for (auto step = 0u; step < Model::SeqStepsPerPage; step++) {
					p.IncStepInSequence(step + page_start, inc, fine);
				}
			} break;

			case Model::SeqEncoderAlts::Random: {
				// idea: a page of random values is created with the amount set to 0
				// turning right increases random amount for the page
				// turning left decreases amount
				// once decreased to 0, turing left ( erases the page and ??) generates a new set of random values
				// then turning right will once again increase the amount

				auto random_range = p.slot.settings.GetRandomOrGlobal(p.GetSelectedChannel());
				if (random_range == 0)
					random_range = Random::Amount::inc;

				random_range *= Model::output_octave_range * 12;
				// TODO: create a buffer of random values upon entry and the encoder selects one
				for (auto step = 0u; step < Model::SeqStepsPerPage; step++) {
					float random_inc = int8_t(std::rand() / 256) / float(-INT8_MIN);
					p.IncStepInSequence(step + page_start, random_range * random_inc, false);
				}
			} break;

			case Model::SeqEncoderAlts::PlayMode:
				if (inc > 0)
					p.ReverseStepOrder(page_start, page_start + Model::SeqStepsPerPage - 1);
				else if (inc < 0)
					p.RandomShuffleStepOrder(page_start, page_start + Model::SeqStepsPerPage - 1);
				break;

			case Model::SeqEncoderAlts::PhaseOffset: {
				if (inc > 0)
					p.RotateStepsRight(page_start, page_start + Model::SeqStepsPerPage - 1);
				else if (inc < 0)
					p.RotateStepsLeft(page_start, page_start + Model::SeqStepsPerPage - 1);

			} break;
		}
	}
	void PaintLeds(const Model::Output::Buffer &) override {
		ClearEncoderLeds(c);
		ClearButtonLeds(c);
		for (auto [page, is_selected] : enumerate(selected_pages))
			c.SetButtonLed(page, is_selected);

		const auto time_now = p.shared.internalclock.TimeNow();
		const auto hang = p.shared.hang.Check(time_now);

		if (hang.has_value()) {
			auto lowest_scene_pressed =
				std::distance(selected_pages.begin(), std::find(selected_pages.begin(), selected_pages.end(), true));
			auto page = p.shared.youngest_scene_button.value_or(lowest_scene_pressed);

			if (page < Model::SeqPages) {
				const auto chan = p.GetSelectedChannel();
				const auto is_gate = p.slot.settings.GetChannelMode(chan).IsGate();

				for (auto i = 0u; i < Model::SeqStepsPerPage; i++) {
					auto color = is_gate ? Palette::GateBlend(p.GetPageValuesGate(page)[i]) :
										   Palette::CvBlend(p.GetPageValuesCv(page)[i]);
					c.SetEncoderLed(i, color);
				}
			}

		} else {
			c.SetEncoderLed(Model::SeqEncoderAlts::Transpose, Palette::Setting::active);
			c.SetEncoderLed(Model::SeqEncoderAlts::Random, Palette::Setting::active);
			c.SetEncoderLed(Model::SeqEncoderAlts::PhaseOffset, Palette::Setting::active);
			c.SetEncoderLed(Model::SeqEncoderAlts::PlayMode, Palette::Setting::active);
		}
	}
};
} // namespace Catalyst2::Ui::Sequencer
