#pragma once

#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
#include "ui/abstract.hh"
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

	SavedSettings<SequencerData, MacroData> settings;

public:
	Interface(Params &params)
		: params{params} {
	}

	void Start() {
		controls.Start();
		StartupAnimation(controls);
		std::srand(controls.ReadSlider() + controls.ReadCv());
		Load();
		ui = GetUi();
		ui->Init();
		params.macro.SelectBank(0);
		params.sequencer.Reset(true);
	}
	void Update() {
		controls.Update();
		params.shared.internalclock.Update();
		params.shared.blinker.Update(params.shared.internalclock.TimeNow());
		params.shared.youngest_scene_button = YoungestSceneButton(controls);
		ui->Common();

		auto next = GetUi();
		ui->Update(next);
		if (next != ui) {
			ui = next;
			ui->Init();
		}

		if (params.shared.do_save) {
			params.shared.do_save = false;
			Save();
		}
	}

	void SetOutputs(const Model::Output::Buffer &outs) {
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
			for (auto i = 0u; i < 48; i++) {
				for (auto but = 0u; but < 8; but++) {
					controls.SetButtonLed(but, !!(i & 0b1));
				}
				controls.Delay(3000 / 48);
			}
		} else {
			for (auto i = 0u; i < 16; i++) {
				controls.SetButtonLed(0, !!(i & 0x01));
				controls.Delay(1000 / 16);
			}
		}
	}
	void Load() {
		if (!settings.read(params.data.sequencer)) {
			params.data.sequencer = SequencerData{};
		}

		if (!settings.read(params.data.macro)) {
			params.data.macro = MacroData{};
		}

		const auto saved_mode = params.shared.data.saved_mode;

		auto &b = controls.button;
		if (b.play.is_high() && b.morph.is_high() && b.fine.is_high()) {
			params.shared.data.saved_mode = Model::Mode::Sequencer;
		} else if (b.bank.is_high() && b.add.is_high() && b.shift.is_high()) {
			params.shared.data.saved_mode = Model::Mode::Macro;
		}

		if (saved_mode != params.shared.data.saved_mode) {
			Save();
		}
		while (b.play.is_high() || b.morph.is_high() || b.fine.is_high() || b.bank.is_high() || b.add.is_high() ||
			   b.shift.is_high())
		{
			__NOP(); // wait until the buttons are released before cont
		}
		params.shared.mode = params.shared.data.saved_mode;
	}
};

} // namespace Catalyst2::Ui
