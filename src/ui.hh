#pragma once

#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "controls.hh"
#include "outputs.hh"
#include "params.hh"
#include "settings.hh"
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
	Outputs outputs;
	Params &params;
	Controls controls;

	Abstract *ui;

	Macro::Ui::Main macro{params.macro, controls};
	Sequencer::Ui::Main sequencer{params.sequencer, controls};

	SavedSettings<Sequencer::Data, Macro::Data> settings;

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

		Abstract *next;
		if (params.shared.data.mode == Model::Mode::Macro) {
			next = &macro;
		} else {
			next = &sequencer;
		}
		ui->Update(next);
		if (next != ui) {
			ui = next;
			ui->Init();
		}
		for (auto [i, sb] : countzip(controls.button.scene)) {
			if (sb.just_went_low()) {
				ui->OnSceneButtonRelease(i);
			}
		}
		controls.ForEachEncoderInc([this](uint8_t encoder, int32_t dir) { ui->OnEncoderInc(encoder, dir); });
		CheckMode();
		Save();
	}

	void SetOutputs(const Model::Output::Buffer &outs) {
		outputs.write(outs);

		if (controls.LedsReady()) {
			ui->PaintLeds(outs);
		}
	}

private:
	void CheckMode() {
		if (params.shared.modeswitcher.Check()) {
			params.shared.modeswitcher.Notify();
			if (params.shared.data.mode == Model::Mode::Macro) {
				params.shared.data.mode = Model::Mode::Sequencer;
				ui = &sequencer;
			} else {
				params.shared.data.mode = Model::Mode::Macro;
				ui = &macro;
				params.macro.SelectBank(0);
			}

			for (auto i = 0u; i < 16; i++) {
				for (auto l = 0u; l < Model::NumScenes; l++) {
					controls.SetButtonLed(l, !!(i & 0b1));
				}
				controls.Delay(1000 / 16);
			}
		}
	}
	void Save() {
		if (params.shared.do_save) {
			params.shared.do_save = false;
			const auto result = params.shared.data.mode == Model::Mode::Macro ? settings.write(params.data.macro) :
																				settings.write(params.data.seq);
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
		if (!settings.read(params.data.seq)) {
			params.data.seq = Sequencer::Data{};
		}

		if (!settings.read(params.data.macro)) {
			params.data.macro = Macro::Data{};
		}

		const auto saved_mode = params.shared.data.mode;

		auto &b = controls.button;
		if (b.play.is_high() && b.morph.is_high() && b.fine.is_high()) {
			params.shared.data.mode = Model::Mode::Sequencer;
		} else if (b.bank.is_high() && b.add.is_high() && b.shift.is_high()) {
			params.shared.data.mode = Model::Mode::Macro;
		}

		if (saved_mode != params.shared.data.mode) {
			(void)settings.write(params.data.macro);
			(void)settings.write(params.data.seq);
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
		}
	}
};

} // namespace Catalyst2::Ui
