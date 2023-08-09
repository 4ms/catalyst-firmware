#include "controls.hh"
#include "debug.hh"
#include "drivers/timekeeper.hh"
#include "hardware_tests/hardware_tests.hh"
#include "macro_seq.hh"
#include "system.hh"
#include "ui.hh"
namespace
{
Catalyst2::System _init;
} // namespace

// DG: general opeations/thoughts
//  After/Before insert... cool idea. Still feeling it out. Can be frustrating, for instance try to make a sequence of
//  steps 1->8 in that order We probably need to calibrate the colors better Ideas for fine/coarse adjustments?

// Bug/missing feature:
// Pressing a scene button by itself should update the outputs
// Holding B and tapping A repeatedly only deletes a step once. But holding A and tapping B repeatedly will delete on
// each tap
// Changing CV does not update the channel buttons
// Make sure if you program a path while CV is applied, that it still makes sense

// Suggested feature:
// Alt + encoder (ie. Randomize) should act upon the current scene if no scene button is pressed?
// Could we blink the channel button that we're fading between (the non-closest one)

void main()
{
	using namespace Catalyst2;

	// Force hardware test for now
	//
	// run_hardware_test();

	Params params;
	UI ui{params};
	MacroSeq macroseq{params};

	mdrivlib::Timekeeper cv_stream(Board::cv_stream_conf, [&macroseq, &ui]() {
		ui.update();
		auto out = macroseq.update();
		ui.set_outputs(out);
	});

	ui.start();
	cv_stream.start();

	while (true) {
		__NOP();
	}
}
