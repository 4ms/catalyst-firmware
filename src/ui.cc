#include "ui.hh"

namespace Catalyst2::Ui
{
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

void MacroMain::OnPlayButtonPress()
{
	if (c->button.shift.is_high())
		recorder.cue_recording();

	if (c->toggle.trig_sense.is_high())
		recorder.reset();
}

void MacroMain::OnEncoderInc(uint8_t encoder, int32_t dir)
{
	const auto scenebdown = c->YoungestSceneButton().has_value();
	const auto fine = c->button.fine.is_high();

	if (scenebdown) {
		for (auto [i, b] : countzip(c->button.scene)) {
			if (b.is_high())
				p->banks.IncChan(i, encoder, dir, fine);
		}
	} else {
		if (p->banks.Path().on_a_scene()) {
			p->banks.IncChan(p->banks.Path().scene_nearest(), encoder, dir, fine);
		} else {
			p->banks.IncChan(p->banks.Path().scene_left(), encoder, dir, fine);
			p->banks.IncChan(p->banks.Path().scene_right(), encoder, dir, fine);
		}
	}
}

void MacroMain::PaintLeds(const Model::OutputBuffer &outs)
{
	c->ClearButtonLeds();
	if (c->YoungestSceneButton().has_value()) {
		for (auto [i, b] : countzip(c->button.scene)) {
			if (b.is_high())
				c->SetButtonLed(i, true);
		}
		const auto scene_to_display = c->YoungestSceneButton().value();
		EncoderDisplayScene(scene_to_display);
	} else {
		EncoderDisplayOutput(outs);
		if (recorder.is_recordering())
			SceneButtonDisplayRecording();
		else {
			const auto l = p->banks.Path().scene_left();
			const auto r = p->banks.Path().scene_right();
			if (l == r)
				c->SetButtonLed(l, true);
			else {
				c->SetButtonLed(l, 1.f - p->pos);
				c->SetButtonLed(r, p->pos);
			}
		}
	}
}

void MacroMain::SceneButtonDisplayRecording()
{
	auto led = static_cast<unsigned>(recorder.capacity_filled() * 8u);
	auto level = (recorder.size() & 0x10) > 0;

	c->SetButtonLed(led, level);
}

void MacroMain::EncoderDisplayScene(Pathway::SceneId scene)
{
	for (auto chan = 0u; chan < Model::NumChans; chan++) {
		auto temp = p->banks.GetChannel(scene, chan);
		Color col = EncoderBlend(temp, p->banks.GetChanMode(chan).IsGate());
		c->SetEncoderLed(chan, col);
	}
}

} // namespace Catalyst2::Ui
