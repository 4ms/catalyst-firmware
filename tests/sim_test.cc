#include "macro_seq.hh"
#include "params.hh"

struct SimUI {
	Catalyst2::Params &params;

	SimUI(Catalyst2::Params &params)
		: params{params} {
		// setup some way to read user input (keyboard, mouse, GUI window...)
		// setup some way to display outputs and leds
	}

	void update() {
		// read inputs, and store into params
	}

	void set_outputs(Catalyst2::Model::OutputBuffer &outs) {
		// display the outputs
		(void)outs;
	}
};

// Test if we could make a simulator
void sim_main() {
	using namespace Catalyst2;

	Params params;
	SimUI ui{params};
	MacroSeq macroseq{params};

	while (true) {
		ui.update();
		auto out = macroseq.Update();
		ui.set_outputs(out);
	}
}
