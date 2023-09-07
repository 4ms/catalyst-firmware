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

// Fine/coarse??

// Display nothing when between scenes
// Insert if blank, add if button is lit
// If hold Insert button down, more presses of scene buttons append

// Rotation?

// Some mode select:
// --Pressing a scene button by itself should update the outputs immediately
// --Pressing a scene button just displays the outputs on the encoders, but outputs don't change
// .. and the channel buttons would blink between the two scenes youre in between

// If a channel is selected by the slider (channel butotn is lit up), then turning encoder sets outputs directly

// Changing CV does not update the channel buttons (CV + slider wraps)

// Make sure if you program a path while CV is applied, that it still makes sense

// Alt + encoder (ie. Randomize) should act upon the current scene if slider is selecting it

// alt + b should go into the main menu thing regardless of which one is pressed first.
// if only one is released we should remain in the menu
// ditto in sequence mode

// maybe hold play to turn on loop?
// do oneshot

// arm recording with trigger where one trigger starts rec and next trigger stops
// one shot should not reset to beginning of recording

// seq length clock divider trigger input

// change the way colors are used for different settings

// simplify the ui states
// set_state()!

// TODO: make color fades appear more linear

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
