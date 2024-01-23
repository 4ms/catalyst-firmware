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
	}
	void Update(Abstract *&interface) override {
		ForEachSceneButtonReleased([this](uint8_t button) { OnSceneButtonRelease(button); });

		if (!c.button.shift.is_high()) {
			return;
		}

		interface = this;
	}
	void OnSceneButtonRelease(uint8_t scene) {
	}
	void PaintLeds(const Model::Output::Buffer &outs) override {
	}
};

} // namespace Catalyst2::Sequencer::Ui

