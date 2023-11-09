#pragma once

#include "controls.hh"

namespace Catalyst2::Ui
{

class Abstract {
public:
	Controls &c;
	Abstract(Controls &c)
		: c{c} {
	}

	virtual void Init() {
	}
	virtual void Update(Abstract *&interface) = 0;
	virtual void Common() = 0;
	virtual void OnSceneButtonRelease(uint8_t button) {
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) {
	}
	virtual void PaintLeds(const Model::OutputBuffer &outs) {
	}
};

} // namespace Catalyst2::Ui
