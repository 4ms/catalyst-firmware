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

// lazygit

// DG: general opeations/thoughts

// Display nothing when between scenes
// Insert if blank, add if button is lit
// If hold Insert button down, more presses of scene buttons append

// Some mode select:
// .. and the channel buttons would blink between the two scenes youre in between

// Make sure if you program a path while CV is applied, that it still makes sense

// Alt + encoder (ie. Randomize) should act upon the current scene if slider is selecting it

// maybe hold play to turn on loop?
// do oneshot

// arm recording with trigger where one trigger starts rec and next trigger stops
// one shot should not reset to beginning of recording

// TODO: make color fades appear more linear

// better way to delete banks

// skip or mute steps in seq

// adding scenes to pathway from other banks?
// each bank should have a path. You just stay in a bank
//  OR:
// Idea:
// Bank+scene button is like paging through bank (preview/view a bank, doesn't changes outputs or current scene)
// Press bank + scene, then press a scene button and that scene&bank goes active to the outputs
// or any A+scene, B+scene uses the current "bank"
// So-- it's not one path per bank , so we need some way to save/recall paths. Maybe Latch button is actually Path?
//
// Idea: (ties into above)
// "Load" or "Slot" (big bank). There are 8 of them, only one is availble at a time
// Alt+Bank + scene button loads it.
// It contains the jack gate/cv types (global to the entire Slot)
// and the voltage range for jacks
// and one pathway
// and one sequence (lengths, start pos, for each channel, and also bank start/pos)

// have slider be more similar to macro mode...

/* october 9th

macro:
shift == random should randomize nearest scened
shift + scene == per bank channel random amount

both: play button stops when playing and restarts when stopped
transpose: post quantizer

pausing keeps fireing gate
also, divided gates keep firing.
gates not firing on external clock.

//transpose and range.

have pause stop where sequence is at and not reset.
maybe clipboards should reset eachother?

have holding multiple seq settings buttons change all pressed buttons together


have gates be activate above 50% instead of 1/65535!!!!!

quantize prior to morph??


*/

volatile uint32_t time;

void main() {
	using namespace Catalyst2;

	// run_hardware_test();

	Params params;
	Ui::Interface ui{params};
	MacroSeq macroseq{params};

	mdrivlib::Timekeeper cv_stream(Board::cv_stream_conf, [&macroseq, &ui]() {
		ui.Update();
		auto out = macroseq.Update();
		ui.SetOutputs(out);
	});

	ui.Start();
	cv_stream.start();
	while (true) {
		__NOP();
	}
}
