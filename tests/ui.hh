// Note this is identical to src/ui.hh (as of the commit where this file is added)
// but we can't just use src/ui.hh in the test project because things like #include "output.hh" loads src/outputs.hh,
// not tests/outputs.hh
// FIXME: figure out how to make test project build system prefer tests/FILE.hh instead of ./FILE.hh when seeing
// #include "FILE.hh"

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

		// load data
		if (!settings.read(params.data.seq)) {
			params.data.seq = Sequencer::Data{};
		}

		if (!settings.read(params.data.macro)) {
			params.data.macro = Macro::Data{};
			params.macro.SelectBank(0);
		}
	}
	void Update() {
		controls.Update();
		params.shared.internalclock.Update();
		ui->Common();

		Abstract *next;
		if (params.mode == Params::Mode::Macro) {
			if (false && params.shared.save.Check()) {
				SaveMacro();
			}
			next = &macro;
		} else {
			if (false && params.sequencer.player.IsPaused() && params.shared.save.Check()) {
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
		if (!settings.write(params.data.macro)) {
			// Flash is damaged?
			PanicWarning();
		}

		// TODO
		for (auto i = 0u; i < 16; i++) {
			controls.SetButtonLed(0, !!(i & 0x01));
			controls.Delay(1000 / 16);
		}
	}
	void SaveSequencer() {
		if (!settings.write(params.data.seq)) {
			// Flash is damaged?
			PanicWarning();
		}

		// TODO
		for (auto i = 0u; i < 16; i++) {
			controls.SetButtonLed(1, !!(i & 0x01));
			controls.Delay(1000 / 16);
		}
	}
	// Non-fatal error. Loudly notify user, but continue operating
	void PanicWarning() {
		for (auto i = 0u; i < 48; i++) {
			for (auto but = 0u; but < 8; but++)
				controls.SetButtonLed(but, !!(i & 0b1));
			controls.Delay(3000 / 48);
		}
	}
	void Load(Params::Mode mode) {
	}
};

} // namespace Catalyst2::Ui
