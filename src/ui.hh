#pragma once

#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
#include "saved_settings.hh"
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
		ui = MainUI();
		ui->Init();
	}

	void Update() {
		controls.Update();
		params.shared.blinker.Update();
		params.shared.youngest_scene_button.Update(controls);

		ui->Common();
		ui->Update();
		if (auto new_ui = ui->NextUi()) {
			ui = new_ui.value();
			ClearEncoderEvents(controls);
			ui->Init();
		}

		if (params.shared.do_save_macro) {
			params.shared.do_save_macro = false;
			SaveMacro();
		}
		if (params.shared.do_save_seq) {
			params.shared.do_save_seq = false;
			SaveSeq();
		}
		if (params.shared.do_save_shared) {
			params.shared.do_save_shared = false;
			SaveShared();
		}
	}

	void SetOutputs(Model::Output::Buffer &outs) {
		controls.Write(outs);

		if (controls.LedsReady()) {
			ui->PaintLeds(outs);
			if (params.shared.blinker.IsSet()) {
				LedBlinker(controls, params.shared.blinker);
			}
			controls.WriteButtonLeds();
		}
	}

private:
	Abstract *MainUI() {
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
		params.data.macro.PreSave();
		if (!settings.write(params.data.macro))
			SaveError();
		params.data.macro.PostLoad();
	}
	void SaveSeq() {
		params.data.sequencer.PreSave();
		if (!settings.write(params.data.sequencer))
			SaveError();
		params.data.sequencer.PostLoad();
	}
	void SaveError() {
		params.shared.blinker.Set(48, 3000);
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
		params.data.sequencer.PostLoad();
		params.data.macro.PostLoad();

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
			controls.SetPlayLed(true);
			controls.Delay(64);
			controls.SetPlayLed(false);
			controls.Delay(64);
		}
		params.shared.mode = params.shared.data.saved_mode;
	}
};

} // namespace Catalyst2::Ui
