#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"

namespace Catalyst2::Sequencer::Ui
{
class SongMode : public Usual {
public:
	using Usual::Usual;
	void Init() override {
		for (auto &s : c.button.scene) {
			s.clear_events();
		}
		p.player.songmode.Cancel();
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonReleased([this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.shift.is_high()) {
			return;
		}

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t scene) {
		p.player.songmode.Queue(scene);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds();
		auto count = p.player.songmode.Size();
		const auto phase = 1.f / (Model::MaxQueuedStartOffsetPages / static_cast<float>(count));
		while (count > 8) {
			count -= 8;
		}
		SetEncoderLedsCount(count, 0, Palette::Pathway::color(phase));
	}
};

} // namespace Catalyst2::Sequencer::Ui
