#pragma once

#include "controls.hh"
#include "params.hh"
#include "seq_common.hh"
#include "seq_reset.hh"

namespace Catalyst2::Ui::Sequencer
{
class SongMode : public Usual {
	Reset reset{p, c};

public:
	using Usual::Usual;
	void Init() override {
		for (auto &s : c.button.scene) {
			s.clear_events();
		}
		p.shared.reset.SetAlarm();
		p.player.songmode.Cancel();
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonJustReleased(c, [this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.play.is_high() || p.player.songmode.Size()) {
			p.shared.reset.SetAlarm();
		}

		if (!c.button.shift.is_high()) {
			if (!p.player.songmode.Size()) {
				p.Stop();
			}
			return;
		}

		if (p.shared.reset.Check()) {
			interface = &reset;
			return;
		}

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t scene) {
		p.player.songmode.Queue(scene);
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
		ClearButtonLeds(c);
		auto count = p.player.songmode.Size();
		const auto phase = 1.f / (Model::Sequencer::MaxQueuedStartOffsetPages / static_cast<float>(count));
		while (count > 8) {
			count -= 8;
		}
		SetEncoderLedsCount(c, count, 0, Palette::Pathway::color(phase));
	}
};

} // namespace Catalyst2::Ui::Sequencer
