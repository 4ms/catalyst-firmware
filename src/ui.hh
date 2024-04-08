#pragma once

#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
#include "ui/abstract.hh"
#include "ui/dac_calibration.hh"
#include "ui/macro.hh"
#include "ui/seq.hh"
#include "util/countzip.hh"

namespace Catalyst2::Ui
{

#ifndef __NOP
volatile uint8_t dummy;
#define __NOP() (void)dummy
#endif

class Interface {
	Params &params;
	Controls controls;

	Abstract *ui;

	Macro::Main macro{params.macro, controls, sequencer};
	Sequencer::Main sequencer{params.sequencer, controls, macro};

	SavedSettings<Catalyst2::Sequencer::Data, Catalyst2::Macro::Data, Catalyst2::Shared::Data> settings;

public:
	Interface(Params &params)
		: params{params} {
	}

	void Start() {
		controls.Start();
		std::srand(controls.ReadSlider() + controls.ReadCv());
		Load();
		if (controls.button.shift.is_high() && controls.button.morph.is_high()) {
			if (Calibration::Dac::Calibrate(params.data.shared.dac_calibration, controls)) {
				SaveShared();
			}
		}
		StartupAnimation(controls);
		params.sequencer.Stop();
		ui = GetUi();
		ui->Init();
	}
	void Update() {
		controls.Update();
		params.shared.internalclock.Update();
		params.shared.blinker.Update(params.shared.internalclock.TimeNow());
		params.shared.youngest_scene_button = YoungestSceneButton(controls);
		ui->Common();

		auto next = GetUi();
		ui->Update(next);
		if (params.shared.do_save_macro) {
			params.shared.do_save_macro = false;
			SaveMacro();
		}
		if (params.shared.do_save_seq) {
			params.shared.do_save_seq = false;
			SaveSeq();
		}
		if (next != ui) {
			ui = next;
			ui->Init();
		}
	}

	void SetOutputs(Model::Output::Buffer &outs) {
		controls.Write(outs);

		if (controls.LedsReady()) {
			ui->PaintLeds(outs);
			if (params.shared.blinker.IsSet()) {
				LedBlinker(controls, params.shared.blinker);
			}
		}
	}

private:
	Abstract *GetUi() {
		if (params.shared.mode == Model::Mode::Macro) {
			return &macro;
		} else {
			return &sequencer;
		}
	}
	void SaveShared() {
		if (!settings.write(params.data.shared))
			SaveError();
	}
	void SaveMacro() {
		if (!settings.write(params.data.macro))
			SaveError();
	}
	void SaveSeq() {
		if (!settings.write(params.data.sequencer))
			SaveError();
	}
	void SaveError() {
		params.shared.blinker.Set(48, 3000, params.shared.internalclock.TimeNow());
	}
	void Load() {
		if (!settings.read(params.data.shared)) {
			params.data.shared = Catalyst2::Shared::Data{};
		}
		if (!settings.read(params.data.sequencer)) {
			params.data.sequencer = Catalyst2::Sequencer::Data{};
		}
		if (!settings.read(params.data.macro)) {
			params.data.macro = Catalyst2::Macro::Data{};
		}

		params.sequencer.Load();
		params.macro.bank.SelectBank(0);

		const auto saved_mode = params.shared.data.saved_mode;

		auto &b = controls.button;
		bool wait = false;
		if (b.play.is_high() && b.morph.is_high() && b.fine.is_high()) {
			params.shared.data.saved_mode = Model::Mode::Sequencer;
			wait = true;
		} else if (b.bank.is_high() && b.add.is_high() && b.shift.is_high()) {
			params.shared.data.saved_mode = Model::Mode::Macro;
			wait = true;
		}

		if (saved_mode != params.shared.data.saved_mode) {
			SaveShared();
		}

		while (wait && (b.play.is_high() || b.morph.is_high() || b.fine.is_high() || b.bank.is_high() ||
						b.add.is_high() || b.shift.is_high()))
		{
			controls.SetPlayLed(controls.Time() & 0x100);
		}
		params.shared.mode = params.shared.data.saved_mode;
	}
};

} // namespace Catalyst2::Ui
