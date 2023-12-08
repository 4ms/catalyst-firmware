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
		ui = &sequencer;
		params.mode = Params::Mode::Sequencer;
		Load();
	}
	void Update() {
		controls.Update();
		params.shared.internalclock.Update();
		ui->Common();

		Abstract *next;
		if (params.mode == Params::Mode::Macro) {
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
			params.mode = params.mode == Params::Mode::Macro ? Params::Mode::Sequencer : Params::Mode::Macro;

			for (auto i = 0u; i < 16; i++) {
				for (auto l = 0u; l < Model::NumScenes; l++) {
					controls.SetButtonLed(l, !!(i & 0b1));
				}
				controls.Delay(1000 / 16);
			}
		}
	}
	void Save() {
		if (controls.button.bank.is_high() && controls.button.morph.just_went_high()) {
			const auto result = params.mode == Params::Mode::Macro ? settings.write(params.data.macro) :
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
			params.macro.SelectBank(0);
		}
	}
};

} // namespace Catalyst2::Ui
