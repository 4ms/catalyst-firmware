#include "ui.hh"

namespace Catalyst2::Ui
{

void GlobalClockDiv(SharedInterface &p, int32_t inc)
{
	p.IncClockDiv(inc);
}

static Color encoder_cv_blend(uint16_t level)
{
	constexpr auto neg = ChannelValue::from_volts(0.f);
	auto temp = level - neg;
	auto c = Palette::Voltage::Positive;
	if (temp < 0) {
		temp *= -2;
		c = Palette::Voltage::Negative;
	}
	const auto phase = (temp / (neg * 2.f));
	return Palette::off.blend(c, phase);
}

static Color encoder_gate_blend(uint16_t level)
{
	if (level == ChannelValue::GateSetFlag)
		return Palette::Gate::Primed;
	if (level == ChannelValue::GateHigh)
		return Palette::Gate::High;
	return Palette::off;
}

Color EncoderBlend(uint16_t level, bool chan_type_gate)
{
	if (chan_type_gate)
		return encoder_gate_blend(level);
	else
		return encoder_cv_blend(level);
}

} // namespace Catalyst2::Ui
