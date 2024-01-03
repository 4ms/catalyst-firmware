#pragma once

#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "controls.hh"
#include "params.hh"
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

	Macro::Ui::Main macro{params.macro, controls};
	Sequencer::Ui::Main sequencer{params.sequencer, controls};

	Controls::SavedSettings<SequencerData, MacroData> settings;

public:
	Interface(Params &params)
		: params{params} {
	}

	void Start() {
		controls.Start();
		std::srand(controls.ReadSlider() + controls.ReadCv());
		Load();
		params.sequencer.player.Stop();
	}
	void Update() {
		controls.Update();
		params.shared.internalclock.Update();
		ui->Common();

		auto next = GetUi();
		ui->Update(next);
		if (next != ui) {
			ui = next;
			if (ui == nullptr) {
				ui = GetUi();
				for (auto i = 0u; i < 16; i++) {
					for (auto l = 0u; l < Model::NumScenes; l++) {
						controls.SetButtonLed(l, !!(i & 0b1));
					}
					controls.Delay(1000 / 16);
				}
			}
			ui->Init();
		}
		for (auto [i, sb] : countzip(controls.button.scene)) {
			if (sb.just_went_low()) {
				ui->OnSceneButtonRelease(i);
			}
		}
		controls.ForEachEncoderInc([this](uint8_t encoder, int32_t dir) { ui->OnEncoderInc(encoder, dir); });
		Save();
	}

	void SetOutputs(const Model::Output::Buffer &outs) {
		controls.Write(outs);

		if (controls.LedsReady()) {
			ui->PaintLeds(outs);
		}
	}

private:
	Abstract *GetUi() {
		if (params.shared.data.mode == Model::Mode::Macro) {
			return &macro;
		} else {
			return &sequencer;
		}
	}
	void Save(bool force = false) {
		if (params.shared.do_save || force) {
			params.shared.do_save = false;

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
	}
	void Load() {
		if (!settings.read(params.data.sequencer)) {
			params.data.sequencer = SequencerData{};
		}

		if (!settings.read(params.data.macro)) {
			params.data.macro = MacroData{};
		}

		const auto saved_mode = params.shared.data.mode;

		auto &b = controls.button;
		if (b.play.is_high() && b.morph.is_high() && b.fine.is_high()) {
			params.shared.data.mode = Model::Mode::Sequencer;
		} else if (b.bank.is_high() && b.add.is_high() && b.shift.is_high()) {
			params.shared.data.mode = Model::Mode::Macro;
		}

		if (saved_mode != params.shared.data.mode) {
			Save(true);
		}
		while (b.play.is_high() || b.morph.is_high() || b.fine.is_high() || b.bank.is_high() || b.add.is_high() ||
			   b.shift.is_high())
		{
			__NOP(); // wait until the buttons are released before cont
		}
		if (params.shared.data.mode == Model::Mode::Macro) {
			ui = &macro;
			params.macro.SelectBank(0);
		} else {
			ui = &sequencer;
			params.sequencer.player.Stop();
		}
	}
};

} // namespace Catalyst2::Ui
