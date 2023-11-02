#pragma once
#include "conf/model.hh"
#include "conf/palette.hh"
#include "controls.hh"
#include "outputs.hh"
#include "params.hh"
#include "util/countzip.hh"

#include "ui/macro.hh"
#include "ui/seq.hh"

namespace Catalyst2::Ui
{
class Interface {
	Outputs outputs;
	Params &params;
	Controls controls;
	bool leds_ready_flag = false;

	mdrivlib::Timekeeper encoder_led_update_task;
	mdrivlib::Timekeeper muxio_update_task;

	Abstract *ui;

	Macro::Ui::Main macro{params.macro, controls};
	Sequencer::Ui::Main sequencer{params.sequencer, controls};

public:
	Interface(Params &params)
		: params{params} {
		encoder_led_update_task.init(Board::encoder_led_task, [this]() {
			controls.WriteToEncoderLeds();
			leds_ready_flag = true;
		});
		muxio_update_task.init(Board::muxio_conf, [this]() { controls.UpdateMuxio(); });
	}
	void Start() {
		encoder_led_update_task.start();
		muxio_update_task.start();
		controls.Start();
		HAL_Delay(2);
		std::srand(controls.ReadSlider() + controls.ReadCv());
		ui = &macro;
	}
	void Update() {
		controls.Update();

		if (controls.jack.trig.just_went_high()) {
			params.shared.clockdivider.Update(params.shared.GetClockDiv());
			if (params.shared.clockdivider.Step())
				params.shared.internalclock.Input();
		}

		ui->Common();

		params.shared.internalclock.Update();

		Abstract *next;
		if (params.mode == Params::Mode::Macro)
			next = &macro;
		else
			next = &sequencer;

		ui->Update(next);

		if (next != ui) {
			ui = next;
			ui->Init();
		}

		if (controls.toggle.mode.just_went_high()) {
			ui = &sequencer;
			params.mode = Params::Mode::Sequencer;
		} else if (controls.toggle.mode.just_went_low()) {
			ui = &macro;
			params.macro.SelectBank(params.macro.GetSelectedBank());
			params.mode = Params::Mode::Macro;
		}

		for (auto [i, sb] : countzip(controls.button.scene)) {
			if (sb.just_went_low())
				ui->OnSceneButtonRelease(i);
		}

		controls.ForEachEncoderInc([this](uint8_t encoder, int32_t dir) { ui->OnEncoderInc(encoder, dir); });
	}

	void SetOutputs(const Model::OutputBuffer &outs) {
		outputs.write(outs);

		if (!leds_ready_flag)
			return;

		leds_ready_flag = false;

		ui->PaintLeds(outs);
	}
};

} // namespace Catalyst2::Ui
