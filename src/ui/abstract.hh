#pragma once

#include "../controls.hh"

namespace Catalyst2::Ui
{

Color EncoderBlend(uint16_t level, bool chan_type_gate);

class Abstract {
public:
	Controls &c;
	Abstract(Controls &c)
		: c{c}
	{}

	virtual void Init()
	{}
	virtual void Update(Abstract *&interface)
	{}
	virtual void Common()
	{}
	virtual void OnSceneButtonRelease(uint8_t button)
	{}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc)
	{}
	virtual void PaintLeds(const Model::OutputBuffer &outs)
	{}
};

} // namespace Catalyst2::Ui