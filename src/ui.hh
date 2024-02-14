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

	SavedSettings<Data::Sequencer, Data::Macro> settings;

public:
	Interface(Params &params)
		: params{params} {
	}

	void Start() {
		controls.Start();
		std::srand(controls.ReadSlider() + controls.ReadCv());
		Load();
		if (controls.button.shift.is_high() && controls.button.morph.is_high()) {
			if (Calibration::Dac::Calibrate(params.data.shared().dac_calibration, controls)) {
				Save();
			}
		}
		StartupAnimation(controls);
		params.macro.SelectBank(0);
		params.sequencer.Reset(true);
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
		if (params.shared.do_save) {
			params.shared.do_save = false;
			Save();
		}
		if (next != ui) {
			ui = next;
			ui->Init();
		}
	}

	void SetOutputs(Model::Output::Buffer &outs) {
		Calibration::Dac::Process(params.data.shared().dac_calibration, outs);
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
	void Save() {
		auto result = settings.write(params.data.macro);
		result &= settings.write(params.data.sequencer);

		if (!result) {
			// save failure
			for (auto i = 0u; i < 48; i++) {
				for (auto but = 0u; but < 8; but++) {
					controls.SetButtonLed(but, !!(i & 0b1));
				}
				controls.Delay(3000 / 48);
			}
		}
	}
	void Load() {
		if (!settings.read(params.data.sequencer)) {
			params.data.sequencer = Data::Sequencer{};
		}
		if (!settings.read(params.data.macro)) {
			params.data.macro = Data::Macro{};
		}

		params.sequencer.Load();

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
			if constexpr (macro_is_smaller)
				settings.write(params.data.macro);
			else
				settings.write(params.data.sequencer);
		}

		while (wait && (b.play.is_high() || b.morph.is_high() || b.fine.is_high() || b.bank.is_high() ||
						b.add.is_high() || b.shift.is_high()))
		{
			__NOP(); // wait until the buttons are released before cont
		}
		params.shared.mode = params.shared.data.saved_mode;
	}
};

} // namespace Catalyst2::Ui
