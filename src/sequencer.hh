#pragma once

#include "channel.hh"
#include "clock.hh"
#include "conf/model.hh"
#include "random.hh"
#include "random_shuffle.hh"
#include "range.hh"
#include "sequence_phaser.hh"
#include "sequencer_player.hh"
#include "sequencer_settings.hh"
#include "sequencer_step.hh"
#include "shared.hh"
#include "song_mode.hh"
#include "util/countzip.hh"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdlib>

namespace Catalyst2::Sequencer
{

struct ChannelData : public std::array<Step, Model::Sequencer::Steps::Max> {
	bool Validate() const {
		auto ret = true;
		for (auto &s : *this) {
			ret &= s.Validate();
		}
		return ret;
	}
};

struct Slot {
	std::array<Sequencer::ChannelData, Model::NumChans> channel;
	Sequencer::Settings::Data settings;
	Player::Data player;
	SongMode::Data songmode;
	Clock::Divider::type clockdiv{};
	Clock::Bpm::Data bpm{};

	bool validate() const {
		auto ret = true;
		for (auto &c : channel) {
			ret &= c.Validate();
		}
		ret &= clockdiv.Validate();
		ret &= settings.Validate();
		ret &= player.Validate();
		ret &= songmode.Validate();
		ret &= bpm.Validate();
		return ret;
	}
};

struct Data {
	std::array<Slot, Model::Sequencer::NumSlots> slot;
	uint8_t startup_slot;

	bool validate() const {
		auto ret = true;
		for (auto &s : slot) {
			ret &= s.validate();
		}
		ret &= startup_slot < slot.size();
		return ret;
	}
};

inline uint8_t SeqPageToStep(uint8_t page) {
	return page * Model::Sequencer::Steps::PerPage;
}

class Interface {
	Data &data;
	uint8_t cur_channel = 0;
	uint8_t cur_page = Model::Sequencer::NumPages;
	struct Clipboard {
		ChannelData cd;
		Settings::Channel cs;
		std::array<Step, Model::Sequencer::Steps::PerPage> page;
	} clipboard;
	uint32_t time_trigged;

public:
	Slot slot;
	Clock::Bpm::Interface seqclock{slot.bpm};
	Shared::Interface &shared;
	Player::Interface player{slot.player, slot.settings, slot.songmode};

	Interface(Data &data, Shared::Interface &shared)
		: data{data}
		, shared{shared} {
	}
	void Load() {
		Load(data.startup_slot);
	}
	void Load(uint8_t slot) {
		this->slot = data.slot[slot];
	}
	void Save(uint8_t slot) {
		data.startup_slot = slot;
		data.slot[slot] = this->slot;
	}
	uint8_t GetStartupSlot() {
		return data.startup_slot;
	}
	void Update(float phase) {
		player.Update(phase, seqclock.GetPhase(), seqclock.Output());
	}

	void Reset(bool stop) {
		seqclock.Reset();
		shared.clockdivider.Reset();
		player.Reset();
		seqclock.Pause(stop);

		// blocks trig for a short period of time
		// TODO: come up with better name
		time_trigged = shared.internalclock.TimeNow();
	}

	void Trig() {
		if (shared.internalclock.TimeNow() - time_trigged >= Clock::BpmToTicks(1200)) {
			if (seqclock.IsInternal()) {
				seqclock.SetExternal(true);
			}
			shared.clockdivider.Update(slot.clockdiv);
			if (shared.clockdivider.Step()) {
				seqclock.Input(shared.internalclock.TimeNow());
			}
		}
	}

	void SelectChannel(uint8_t chan) {
		cur_channel = chan;
	}
	uint8_t GetSelectedChannel() {
		return cur_channel;
	}
	void DeselectChannel() {
		cur_channel = Model::NumChans;
	}
	bool IsChannelSelected() {
		return cur_channel < Model::NumChans;
	}
	void SelectPage(uint8_t page) {
		cur_page = page;
	}
	uint8_t GetSelectedPage() {
		return cur_page;
	}
	void DeselectPage() {
		cur_page = Model::Sequencer::NumPages;
	}
	bool IsPageSelected() {
		return cur_page < Model::Sequencer::NumPages;
	}
	void IncStep(uint8_t step, int32_t inc, bool fine) {
		IncStepInSequence(StepOnPageToStep(step), inc, fine);
	}
	void IncStepInSequence(uint8_t step, int32_t inc, bool fine) {
		if (step >= Model::Sequencer::Steps::Max)
			return;
		auto &c = slot.channel[cur_channel][step];
		if (slot.settings.GetChannelMode(cur_channel).IsGate()) {
			if (fine) {
				c.IncTrigDelay(inc);
			} else {
				c.IncGate(inc);
			}
		} else {
			c.IncCv(inc, fine, slot.settings.GetRange(cur_channel));
		}
	}
	void RotateStepsLeft(uint8_t first_step, uint8_t last_step) {
		auto &all_steps = slot.channel[cur_channel];
		auto steps = std::span<Step>{&all_steps[first_step], &all_steps[last_step + 1]};
		std::rotate(steps.begin(), steps.begin() + 1, steps.end());
	}
	void RotateStepsRight(uint8_t first_step, uint8_t last_step) {
		auto &all_steps = slot.channel[cur_channel];
		auto steps = std::span<Step>{&all_steps[first_step], &all_steps[last_step + 1]};
		std::rotate(steps.rbegin(), steps.rbegin() + 1, steps.rend());
	}
	void RandomShuffleStepOrder(uint8_t first_step, uint8_t last_step) {
		auto &all_steps = slot.channel[cur_channel];
		auto steps = std::span<Step>{&all_steps[first_step], &all_steps[last_step + 1]};
		Catalyst2::random_shuffle(steps.begin(), steps.end());
	}
	void ReverseStepOrder(uint8_t first_step, uint8_t last_step) {
		auto &all_steps = slot.channel[cur_channel];
		auto steps = std::span<Step>{&all_steps[first_step], &all_steps[last_step + 1]};
		std::reverse(steps.begin(), steps.end());
	}
	void IncStepModifier(uint8_t step, int32_t inc) {
		step = StepOnPageToStep(step);
		slot.channel[cur_channel][step].IncMorphRetrig(inc);
	}
	void IncStepProbability(uint8_t step, int32_t inc) {
		step = StepOnPageToStep(step);
		slot.channel[cur_channel][step].IncProbability(inc);
	}

	Step GetRelativeStep(uint8_t chan, int8_t relative_pos) {
		const auto step = player.GetRelativeStep(chan, relative_pos);
		return slot.channel[chan][step];
	}
	auto GetRelativeStepCv(uint8_t chan, int8_t relative_pos) {
		const auto s = GetRelativeStep(chan, relative_pos);
		auto r = player.randomvalue.ReadRelative(chan, relative_pos, s.ReadProbability());
		return s.ReadCv(r * slot.settings.GetRandomOrGlobal(chan));
	}
	auto GetRelativeStepGate(uint8_t chan, int8_t relative_pos) {
		const auto s = GetRelativeStep(chan, relative_pos);
		auto r = player.randomvalue.ReadRelative(chan, relative_pos, s.ReadProbability());
		return s.ReadGate(r * slot.settings.GetRandomOrGlobal(chan));
	}
	Step GetStep(uint8_t step) {
		return slot.channel[cur_channel][step];
	}
	void CopySequence() {
		clipboard.cd = slot.channel[cur_channel];
		clipboard.cs = slot.settings.Copy(cur_channel);
	}
	void PasteSequence() {
		slot.channel[cur_channel] = clipboard.cd;
		slot.settings.Paste(cur_channel, clipboard.cs);
	}
	void CopyPage(uint8_t page) {
		for (auto i = 0u; i < Model::Sequencer::Steps::PerPage; i++) {
			clipboard.page[i] = slot.channel[cur_channel][SeqPageToStep(page) + i];
		}
	}
	void PastePage(uint8_t page) {
		for (auto i = 0u; i < Model::Sequencer::Steps::PerPage; i++) {
			slot.channel[cur_channel][SeqPageToStep(page) + i] = clipboard.page[i];
		}
	}

private:
	uint8_t StepOnPageToStep(uint8_t step_on_page) {
		const auto page = IsPageSelected() ? GetSelectedPage() : player.GetPlayheadPage(cur_channel);
		return step_on_page + (page * Model::Sequencer::Steps::PerPage);
	}
};

} // namespace Catalyst2::Sequencer
