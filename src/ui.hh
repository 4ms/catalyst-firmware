#pragma once
#include "clockdivider.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "intclock.hh"
#include "outputs.hh"
#include "params.hh"
#include "randompool.hh"
#include "recorder.hh"
#include "util/countzip.hh"

namespace Catalyst2::Ui
{

Color EncoderBlend(uint16_t level, bool chan_type_gate);

class Interface {
public:
	static constexpr uint32_t hang_time_ms = 2000;
	static inline uint32_t display_hang_time;
	static inline uint8_t hang_on;

	static inline Params *p;
	static inline Controls *c;
	static inline ClockDivider clockdivider;
	static inline InternalClock<Board::cv_stream_hz> internalclock;

	// defualt do nothing, can override
	virtual void Init(){};
	virtual void OnSceneButtonRelease(uint8_t scene){};
	virtual void OnPlayButtonPress(){};

	// must override
	virtual bool Update() = 0;
	virtual void OnEncoderInc(uint8_t encoder, int32_t dir) = 0;
	virtual void PaintLeds(const Model::OutputBuffer &outs) = 0;
};

class SeqMain : public Interface {
public:
	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}
	void OnSceneButtonRelease(uint8_t scene) override
	{
		if (!p->seq.IsChanSelected())
			return;

		p->seq.SelPage(scene);
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		if (!p->seq.IsChanSelected())
			return;

		const auto fine = c->button.fine.is_high();
		const auto page = p->seq.IsPageSelected() ? p->seq.GetSelPage() : p->seq.GetStepPage(p->seq.GetSelChan());
		const auto step = (page * Model::SeqStepsPerPage) + encoder;
		p->seq.IncStep(step, dir, fine);
	}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();

		if (p->seq.IsChanSelected()) {
			const auto chan = p->seq.GetSelChan();
			const uint8_t led = p->seq.GetStep(chan) & 0x7;
			const auto page = p->seq.IsPageSelected() ? p->seq.GetSelPage() : p->seq.GetStepPage(chan);
			const auto stepoffset = page * Model::SeqStepsPerPage;

			for (auto i = 0u; i < Model::NumChans; i++) {
				if (i == led && page == p->seq.GetStepPage(chan))
					c->SetEncoderLed(led, Palette::seqhead);
				else
					c->SetEncoderLed(
						i, EncoderBlend(p->seq.GetStepValue(chan, stepoffset + i), p->seq.GetChanMode(chan).IsGate()));
			}
			if (p->seq.IsPageSelected())
				c->SetButtonLed(page, ((HAL_GetTick() >> 8) & 1) > 0);
			else
				c->SetButtonLed(page, true);

		} else {
			EncoderDisplayOutput(outs);
		}
	}

	static void Common()
	{
		if (c->jack.trig.just_went_high()) {
			clockdivider.Update();
		}

		if (c->jack.reset.is_high()) {
			if (c->toggle.trig_sense.is_high())
				internalclock.Reset();
			else
				clockdivider.Reset();

			p->seq.Reset();
			return;
		}

		bool step;

		if (c->toggle.trig_sense.is_high()) {
			internalclock.update();
			step = internalclock.step();
		} else {
			step = clockdivider.Step();
		}

		if (step)
			p->seq.Step();
	}

	static void EncoderDisplayOutput(const Model::OutputBuffer &buf)
	{
		for (auto [chan, val] : countzip(buf)) {
			Color col = EncoderBlend(val, p->seq.GetChanMode(chan).IsGate());
			c->SetEncoderLed(chan, col);
		}
	}

private:
	static bool RequestModeUpdate()
	{
		if (!c->toggle.mode.is_high())
			return true;

		if (c->button.shift.is_high())
			return true;

		if (c->button.bank.is_high())
			return true;

		return false;
	}
};

class SeqBank : public SeqMain {
public:
	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		if (c->button.shift.is_high()) {
			// change all channgels.
			auto cm = p->seq.GetChanMode(encoder);
			cm.Inc(dir);
			for (auto i = 0u; i < Model::NumChans; i++) {
				p->seq.SetChanMode(i, cm);
				p->quantizer_bank[i].load_scale(cm.GetScale());
			}
		} else {
			p->seq.IncChanMode(encoder, dir);
			p->quantizer_bank[encoder].load_scale(p->seq.GetChanMode(encoder).GetScale());
		}
	}

	void OnSceneButtonRelease(uint8_t scene) override
	{
		p->seq.DeselPage();

		if (scene == p->seq.GetSelChan())
			p->seq.DeselChan();
		else
			p->seq.SelChan(scene);
	}

	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();
		c->SetButtonLed(p->seq.GetSelChan(), true);

		for (auto i = 0u; i < Model::NumChans; i++)
			c->SetEncoderLed(i, p->seq.GetChanMode(i).GetColor());
	}

private:
	static bool RequestModeUpdate()
	{
		if (!c->button.bank.is_high())
			return true;

		if (c->button.shift.is_high())
			return true;

		return false;
	}
};

class MacroMain : public Interface {

public:
	static inline Recorder recorder;

	void Init() override
	{
		c->button.copy.clear_events();
	}

	bool Update() override
	{
		Common();

		if (p->override_output.has_value() && c->button.copy.just_went_high())
			p->banks.CopySceneToClipboard(p->override_output.value());

		return RequestModeUpdate();
	}
	void OnPlayButtonPress() override;
	void OnEncoderInc(uint8_t encoder, int32_t dir) override;
	void PaintLeds(const Model::OutputBuffer &outs) override;

	static void SceneButtonDisplayRecording();
	static void EncoderDisplayScene(Pathway::SceneId scene);
	static void Common()
	{
		if (c->jack.trig.just_went_high()) {
			if (recorder.is_cued()) {
				clockdivider.Reset();
				recorder.record();
			} else {
				clockdivider.Update();
				if (clockdivider.Step())
					recorder.reset();
			}
		}
		p->pos = recorder.update(c->ReadSlider() + c->ReadCv()) / 4096.f;
		p->override_output = c->YoungestSceneButton();
	}
	static void EncoderDisplayOutput(const Model::OutputBuffer &buf)
	{
		for (auto [chan, val] : countzip(buf)) {
			Color col = EncoderBlend(val, p->banks.GetChanMode(chan).IsGate());
			c->SetEncoderLed(chan, col);
		}
	}

private:
	static bool RequestModeUpdate()
	{
		if (c->toggle.mode.is_high())
			return true;

		if (c->button.add.is_high())
			return true;

		if (c->button.bank.is_high())
			return true;

		if (c->button.copy.is_high())
			return true;

		if (c->button.shift.is_high())
			return true;

		return false;
	}
};

class MacroAdd : public MacroMain {
	static inline bool first = false;

public:
	void OnPlayButtonPress() override{};						/* do nothing */
	void OnEncoderInc(uint8_t encoder, int32_t dir) override{}; /* do nothing */

	void Init() override
	{
		first = true;
		c->button.shift.clear_events();
	}
	bool Update() override
	{
		MacroMain::Common();

		if (c->button.shift.just_went_high()) {
			auto &path = p->banks.Path();

			if (path.on_a_scene() || (path.scene_left() == path.scene_right()))
				path.remove_scene_nearest();
		}

		return RequestModeUpdate();
	}

	void OnSceneButtonRelease(uint8_t scene) override
	{
		auto &path = p->banks.Path();

		if (c->button.shift.is_high()) {
			if (path.on_a_scene()) {
				if (path.scene_nearest() == scene)
					path.remove_scene_nearest();
			} else {
				if (path.scene_left() == scene)
					path.remove_scene_left();
				else if (path.scene_right() == scene)
					path.remove_scene_right();
			}
			return;
		}

		if (!first) {
			path.insert_scene(scene, true);
			return;
		}

		first = false;

		if (path.on_a_scene())
			path.replace_scene(scene);
		else
			path.insert_scene(scene, false);
	}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();

		auto &path = p->banks.Path();

		auto count = path.size();
		const auto phase = 1.f / (path.MaxPoints / static_cast<float>(count));

		while (count > 8)
			count -= 8;

		c->SetEncoderLedsCount(count, 0, Palette::green.blend(Palette::red, phase));

		if (path.on_a_scene()) {
			c->SetButtonLed(path.scene_nearest(), true);
		} else {
			c->SetButtonLed(path.scene_left(), true);
			c->SetButtonLed(path.scene_right(), true);
		}
	}

private:
	static bool RequestModeUpdate()
	{
		if (!c->button.add.is_high() && !c->button.shift.is_high())
			return true;

		return false;
	}
};

class MacroMorph : public MacroMain {
public:
	void OnPlayButtonPress() override{}; /* do nothing */

	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		if (encoder != 6)
			return;

		p->morph_step += (1.f / 100.f) * dir;
		p->morph_step = std::clamp(p->morph_step, 0.f, 1.f);
	}

	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}

	void OnSceneButtonRelease(uint8_t scene) override
	{
		p->banks.PasteToScene(scene);
	}

	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearEncoderLeds();

		auto col = Palette::grey.blend(Palette::red, p->morph_step);
		if (p->morph_step == 0.f)
			col = Palette::green;

		c->SetEncoderLed(6, col);
	}

private:
	static bool RequestModeUpdate()
	{
		if (!c->button.copy.is_high())
			return true;

		return false;
	}
};

class MacroBank : public MacroMain {
public:
	void OnPlayButtonPress() override{}; /* do nothing */

	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		if (c->button.shift.is_high()) {
			// change all channgels.
			auto cm = p->banks.GetChanMode(encoder);
			for (auto i = 0u; i < Model::NumChans; i++) {
				cm.Inc(dir);
				p->banks.SetChanMode(i, cm);
				p->quantizer_bank[i].load_scale(cm.GetScale());
			}
		} else {
			p->banks.IncChanMode(encoder, dir);
			p->quantizer_bank[encoder].load_scale(p->banks.GetChanMode(encoder).GetScale());
		}
	}

	void OnSceneButtonRelease(uint8_t scene) override
	{
		p->banks.SelBank(scene);
	}

	bool Update() override
	{
		return RequestModeUpdate();
	}

	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();
		c->SetButtonLed(p->banks.GetSelBank(), true);
		for (auto i = 0u; i < Model::NumChans; i++) {
			const auto col = p->banks.GetChanMode(i).GetColor();
			c->SetEncoderLed(i, col);
		}
	}

private:
	static bool RequestModeUpdate()
	{
		if (!c->button.bank.is_high())
			return true;

		if (c->button.shift.is_high())
			return true;

		return false;
	}
};

class Settings : public Interface {
public:
	static constexpr uint32_t hang_time_ms = 2000;
	static inline uint32_t display_hang_time;
	static inline uint8_t hang_on;

	void OnSceneButtonRelease(uint8_t scene) override{}; /* do nothing */
	void OnPlayButtonPress() override{};				 /* do nothing */

	void Init() override
	{
		hang_on = 0xff;
	};

	static void Common()
	{
		if (p->mode == Params::Mode::Macro)
			MacroMain::Common();
		else
			SeqMain::Common();
	}
};

class GlobalSettings : public Settings {
public:
	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		display_hang_time = HAL_GetTick();

		if (encoder == Model::EncoderAlts::ClockDiv) {
			if (p->mode == Params::Mode::Sequencer && c->toggle.trig_sense.is_high()) {
				internalclock.bpm_inc(dir, false);
			} else {
				clockdivider.IncDiv(hang_on == 0xff ? 0 : dir);
				hang_on = encoder;
			}
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();
		c->ClearEncoderLeds();

		auto time_now = HAL_GetTick();

		if (time_now - display_hang_time >= hang_time_ms)
			hang_on = 0xff;

		if (hang_on != 0xff) {
			if (hang_on == Model::EncoderAlts::ClockDiv) {
				c->SetEncoderLedsAddition(clockdivider.GetDiv(), Palette::blue);
			}
		} else {
			auto col = Palette::blue;
			if (p->mode == Params::Mode::Sequencer && c->toggle.trig_sense.is_high()) {
				if (p->seq.GetClock())
					col = Palette::green;
				else
					col = Palette::off;
			}
			c->SetEncoderLed(Model::EncoderAlts::ClockDiv, col);
		}
	}

private:
	bool RequestModeUpdate()
	{
		if (!c->button.bank.is_high() && !c->button.shift.is_high())
			return true;

		return false;
	}
};

class SeqSettings : public Settings {
public:
	void Init() override
	{
		Settings::Init();

		if (!p->seq.IsChanSelected())
			p->seq.SelChan(0);
	}
	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{
		display_hang_time = HAL_GetTick();

		if (encoder == Model::EncoderAlts::Random) {
			RandomPool::RandomizeSequenceChannel(p->seq.GetSelChan());
			return;
		}

		if (encoder == Model::EncoderAlts::PlayMode) {
			p->seq.AdjPlayMode(dir);
			return;
		}

		// this makes it so the value isnt affected on the first encoder click
		dir = hang_on == 0xff ? 0 : dir;

		if (encoder == Model::EncoderAlts::StartOffset) {
			p->seq.AdjStartOffset(dir);
			hang_on = encoder;
			return;
		}

		if (encoder == Model::EncoderAlts::SeqLength) {
			p->seq.AdjLength(dir);
			hang_on = encoder;
			return;
		}

		if (encoder == Model::EncoderAlts::ClockDiv) {
			p->seq.ClockDiv().IncDiv(dir);
			hang_on = encoder;
			return;
		}
	}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();
		c->ClearEncoderLeds();
		c->SetButtonLed(p->seq.GetSelChan(), true);

		auto time_now = HAL_GetTick();

		if (time_now - display_hang_time >= hang_time_ms)
			hang_on = 0xff;

		if (hang_on != 0xff) {
			if (hang_on == Model::EncoderAlts::StartOffset) {
				auto l = p->seq.GetStartOffset();
				const auto col = Palette::SeqPage[l / Model::SeqStepsPerPage];
				while (l >= 8)
					l -= 8;
				c->SetEncoderLed(l, col);
			} else if (hang_on == Model::EncoderAlts::SeqLength) {
				auto l = p->seq.GetLength() % Model::SeqStepsPerPage;
				l = l == 0 ? Model::SeqStepsPerPage : l;
				c->SetEncoderLedsCount(l, 0, Palette::magenta);
				l = (p->seq.GetLength() - 1) / Model::SeqStepsPerPage;
				c->SetButtonLedsCount(l + 1, true);
			} else if (hang_on == Model::EncoderAlts::ClockDiv) {
				const auto div = p->seq.ClockDiv().GetDiv();
				c->SetEncoderLedsAddition(div, Palette::blue);
			}
		} else {
			c->SetEncoderLed(Model::EncoderAlts::StartOffset, Palette::seqhead);
			PlayModeLedAnnimation(time_now);
			c->SetEncoderLed(Model::EncoderAlts::SeqLength, Palette::seqhead);
			c->SetEncoderLed(Model::EncoderAlts::ClockDiv, Palette::blue);
			c->SetEncoderLed(Model::EncoderAlts::Random, Palette::from_raw(RandomPool::GetSeed(p->seq.GetSelChan())));
		}
	}

private:
	static void PlayModeLedAnnimation(uint32_t time_now)
	{
		auto phase = (time_now & 1023) / 1023.f;
		Color col;
		const auto pm = p->seq.GetPlayMode();

		if (pm == Sequencer::PlayMode::Sequential)
			col = Palette::green.blend(Palette::blue, phase);
		else if (pm == Sequencer::PlayMode::PingPong) {
			if (phase < .5f) {
				phase *= 2.f;
				col = Palette::green.blend(Palette::blue, phase);
			} else {
				phase -= .5f;
				phase *= 2.f;
				col = Palette::blue.blend(Palette::green, phase);
			}
		} else {
			col = Palette::from_raw(time_now >> 6);
		}
		c->SetEncoderLed(Model::EncoderAlts::PlayMode, col);
	}

	bool RequestModeUpdate()
	{
		if (!c->button.shift.is_high())
			return true;

		return false;
	}
};

class MacroSettings : public Settings {
public:
	bool Update() override
	{
		Common();
		return RequestModeUpdate();
	}
	void OnEncoderInc(uint8_t encoder, int32_t dir) override
	{}
	void PaintLeds(const Model::OutputBuffer &outs) override
	{
		c->ClearButtonLeds();
		c->ClearEncoderLeds();
	}

private:
	bool RequestModeUpdate()
	{
		if (!c->button.shift.is_high())
			return true;

		return false;
	}
};

class UserInterface {
	Interface *interface;
	MacroMain macromain;
	MacroAdd macroadd;
	MacroMorph macromorph;
	MacroBank macrobank;

	SeqMain seqmain;
	SeqBank seqbank;

	GlobalSettings globalsettings;
	SeqSettings seqsettings;

	Outputs outputs;
	Params &params;
	Controls controls;
	bool leds_ready_flag = false;

	mdrivlib::Timekeeper encoder_led_update_task;
	mdrivlib::Timekeeper muxio_update_task;

public:
	UserInterface(Params &params)
		: params{params}
	{
		interface->p = &params;
		interface->c = &controls;
		interface = &macromain;

		encoder_led_update_task.init(Board::encoder_led_task, [this]() {
			controls.WriteToEncoderLeds();
			leds_ready_flag = true;
		});
		muxio_update_task.init(Board::muxio_conf, [this]() { controls.UpdateMuxio(); });
	}
	void Start()
	{
		encoder_led_update_task.start();
		muxio_update_task.start();
		controls.Start();
		HAL_Delay(2);
		RandomPool::Init(controls.ReadSlider() + controls.ReadCv());
	}
	void Update()
	{
		controls.Update();

		if (interface->Update()) {
			UpdateInterface();
			interface->Init();
		}

		if (controls.button.play.just_went_high())
			interface->OnPlayButtonPress();

		for (auto [i, sb] : countzip(controls.button.scene)) {
			if (sb.just_went_low())
				interface->OnSceneButtonRelease(i);
		}

		controls.ForEachEncoderInc([this](uint8_t encoder, int32_t dir) { interface->OnEncoderInc(encoder, dir); });
	}

	void SetOutputs(const Model::OutputBuffer &outs)
	{
		outputs.write(outs);

		if (!leds_ready_flag)
			return;

		leds_ready_flag = false;

		interface->PaintLeds(outs);
	}

private:
	void UpdateInterface()
	{
		if (controls.toggle.mode.is_high())
			params.mode = Params::Mode::Sequencer;
		else
			params.mode = Params::Mode::Macro;

		if (params.mode == Params::Mode::Macro)
			UpdateInterfaceMacro();
		else
			UpdateInterfaceSeq();
	}
	void UpdateInterfaceMacro()
	{
		if (controls.button.bank.is_high() && controls.button.shift.is_high())
			interface = &globalsettings;
		else if (controls.button.bank.is_high())
			interface = &macrobank;
		else if (controls.button.shift.is_high())
			;
		else if (controls.button.add.is_high())
			interface = &macroadd;
		else if (controls.button.copy.is_high())
			interface = &macromorph;
		else
			interface = &macromain;
	}
	void UpdateInterfaceSeq()
	{
		if (controls.button.bank.is_high() && controls.button.shift.is_high())
			interface = &globalsettings;
		else if (controls.button.bank.is_high())
			interface = &seqbank;
		else if (controls.button.shift.is_high())
			interface = &seqsettings;
		else
			interface = &seqmain;
	}
};

} // namespace Catalyst2::Ui
