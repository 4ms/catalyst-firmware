#pragma once

#include "../controls.hh"

namespace Catalyst2::Ui
{

class Abstract {
public:
	Controls &c;
	Abstract(Controls &c)
		: c{c} {
	}

	virtual void Init() = 0;
	virtual void Update(Abstract *&interface) = 0;
	virtual void Common() = 0;
	virtual void OnSceneButtonRelease(uint8_t button) = 0;
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) = 0;
	virtual void PaintLeds(const Model::OutputBuffer &outs) = 0;
};

} // namespace Catalyst2::Ui