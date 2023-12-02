#pragma once

#include "conf/model.hh"
#include "controls.hh"
#include "outputs.hh"
#include "params.hh"
#include "ui/macro.hh"
#include "ui/seq.hh"
#include "util/countzip.hh"

namespace Catalyst2::Ui
{

class Interface {
	Outputs outputs;
	Params &params;
	Controls controls;

	Abstract *ui;

	Macro::Ui::Main macro{params.macro, controls};
	Sequencer::Ui::Main sequencer{params.sequencer, controls};

public:
	Interface(Params &params)
		: params{params} {
	}
	void Start() {
		controls.Start();
		std::srand(controls.ReadSlider() + controls.ReadCv());
		ui = &macro;

		params.shared.internalclock.SetExternal(!controls.toggle.trig_sense.is_high());
	}
	void Update() {
		controls.Update();

		if (controls.jack.trig.just_went_high()) {
			params.shared.clockdivider.Update(params.shared.clockdiv);
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
			params.sequencer.player.Stop();
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

	void SetOutputs(const Model::Output::Buffer &outs) {
		outputs.write(outs);

		if (controls.LedsReady())
			ui->PaintLeds(outs);
	}
};

} // namespace Catalyst2::Ui
