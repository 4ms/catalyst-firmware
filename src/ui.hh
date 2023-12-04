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

		// load data
		auto data = controls.Load(sizeof(MacroMode::Data), Model::ModeSwitch::Macro);
		if (data != nullptr) {
			params.data.macro = *(MacroMode::Data *)data;
		}
		data = controls.Load(sizeof(SeqMode::Data), Model::ModeSwitch::Sequence);
		if (data != nullptr) {
			params.data.seq = *(SeqMode::Data *)data;
		}
	}
	void Update() {
		controls.Update();
		params.shared.internalclock.Update();
		ui->Common();

		Abstract *next;
		if (params.mode == Params::Mode::Macro) {
			if (true && params.shared.save.Check()) {
				SaveMacro();
			}
			next = &macro;
		} else {
			if (params.sequencer.player.IsPaused() && params.shared.save.Check()) {
				SaveSequencer();
			}
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
	}

	void SetOutputs(const Model::Output::Buffer &outs) {
		outputs.write(outs);

		if (controls.LedsReady()) {
			ui->PaintLeds(outs);
		}
	}

private:
	void CheckMode() {
		if (controls.button.shift.just_went_high() && controls.button.play.is_high() && controls.button.bank.is_high())
		{
			params.shared.reset.Notify(false);
			if (params.mode == Params::Mode::Sequencer) {
				SaveSequencer();
				params.mode = Params::Mode::Macro;
				ui = &macro;
				params.macro.SelectBank(params.macro.GetSelectedBank());
			} else {
				params.sequencer.player.Stop();
				SaveMacro();
				params.mode = Params::Mode::Sequencer;
				ui = &sequencer;
				controls.button.play.clear_events();
			}
		}
	}
	void SaveMacro() {
		controls.Save((uint32_t *)&params.data.macro, sizeof(MacroMode::Data), Model::ModeSwitch::Macro);
	}
	void SaveSequencer() {
		controls.Save((uint32_t *)&params.data.seq, sizeof(SeqMode::Data), Model::ModeSwitch::Sequence);
	}
	void Load(Params::Mode mode) {
	}
};

} // namespace Catalyst2::Ui
