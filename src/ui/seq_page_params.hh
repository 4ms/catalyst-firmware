#pragma once

#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "sequencer.hh"
#include <climits>

namespace Catalyst2::Ui::Sequencer
{
class PageParams : public Usual {
	std::array<bool, Model::Sequencer::NumPages> selected_pages{};
	bool is_latching_scene_buts = false;
	uint8_t displayed_page = 0;

public:
	using Usual::Usual;
	void Init() override {
		p.shared.hang.Cancel();
		if (!p.IsChannelSelected()) {
			p.SelectChannel();
		}

		is_latching_scene_buts = false;
		for (auto [page, selected] : enumerate(selected_pages)) {
			selected = c.button.scene[page].is_pressed();
			if (selected)
				displayed_page = page;
		}
	}
	void Update() override {
		if (!c.button.shift.is_high() && !p.shared.youngest_scene_button.has_value()) {
			SwitchUiMode(main_ui);
			return;
		}

		// On button press events, extend hang time if it's already hanging
		// Display the most recently pressed button
		ForEachSceneButtonJustPressed(c, [this](auto page) {
			displayed_page = page;
			if (p.shared.hang.Check()) {
				p.shared.hang.Set({}, p.shared.internalclock.TimeNow());
			}
		});

		auto num_pressed = 0u;
		ForEachSceneButtonPressed(c, [&, this](auto page) {
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
	}

	void OnEncoderInc(uint8_t encoder, int32_t inc, uint32_t page) {
		const auto time_now = p.shared.internalclock.TimeNow();
		using namespace Model::Sequencer;
		const auto page_start = page * Steps::PerPage;
		p.shared.hang.Set(encoder, time_now);

		switch (encoder) {
			case EncoderAlts::Transpose: {
				const auto fine = c.button.fine.is_high();
				const auto page_start = page * Steps::PerPage;
				for (auto step = 0u; step < Steps::PerPage; step++) {
					p.IncStepInSequence(step + page_start, inc, fine);
				}
			} break;

			case EncoderAlts::Random: {
				// idea: a page of random values is created with the amount set to 0
				// turning right increases random amount for the page
				// turning left decreases amount
				// once decreased to 0, turing left ( erases the page and ??) generates a new set of random values
				// then turning right will once again increase the amount

				auto random_range = p.slot.settings.GetRandomOrGlobal(p.GetSelectedChannel());
				if (random_range == 0.f)
					random_range = Random::Amount::def;

				random_range *= Model::output_octave_range * 12;
				// TODO: create a buffer of random values upon entry and the encoder selects one
				for (auto step = 0u; step < Steps::PerPage; step++) {
					float random_inc = MathTools::map_value(std::rand(), 0, RAND_MAX, -1.f, +1.f);
					p.IncStepInSequence(step + page_start, std::round(random_range * random_inc), false);
				}
			} break;

			case EncoderAlts::PlayMode:
				if (inc > 0)
					p.ReverseStepOrder(page_start, page_start + Steps::PerPage - 1);
				else if (inc < 0)
					p.RandomShuffleStepOrder(page_start, page_start + Steps::PerPage - 1);
				break;

			case EncoderAlts::PhaseOffset: {
				if (inc > 0)
					p.RotateStepsRight(page_start, page_start + Steps::PerPage - 1);
				else if (inc < 0)
					p.RotateStepsLeft(page_start, page_start + Steps::PerPage - 1);

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

		using namespace Model::Sequencer;

		if (hang.has_value()) {
			// auto lowest_scene_pressed =
			// 	std::distance(selected_pages.begin(), std::find(selected_pages.begin(), selected_pages.end(), true));
			// auto page = p.shared.youngest_scene_button.value_or(lowest_scene_pressed);
			// displayed_page = 3;
			auto page = displayed_page;

			if (page < NumPages) {
				PaintStepValues(page);
			}

		} else {
			c.SetEncoderLed(EncoderAlts::Transpose, Palette::Setting::active);
			c.SetEncoderLed(EncoderAlts::Random, Palette::Setting::active);
			c.SetEncoderLed(EncoderAlts::PhaseOffset, Palette::Setting::active);
			c.SetEncoderLed(EncoderAlts::PlayMode, Palette::Setting::active);
		}
	}
};
} // namespace Catalyst2::Ui::Sequencer
