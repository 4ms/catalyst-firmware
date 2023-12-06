#include "ui.hh"

#include "../src/macro_seq.hh"
#include "controls.hh"
#include "params.hh"

// Test if we could make a simulator
void sim_main() {
	using namespace Catalyst2;

	Params params;
	Ui::Interface ui{params};
	MacroSeq macroseq{params};

	ui.Start();

	while (true) {
		ui.Update();
		auto out = macroseq.Update();
		ui.SetOutputs(out);
	}
}
