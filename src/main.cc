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

// explore sequencing banks.
// start with bank 1 but allow increasing to all 8 banks
// handle changin bank length and startpoint while seq is running into next banks
// look into dimming leds

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

// copy and pasting scenes.

// scene random amount 0% not working fully

// interp array (utils)

// Idea:
// a channel can only be a gate or a cv, not both (per bank)
//
// Idea:
// globally, an output jack can be cv or gate. the value in the scenes get translated to voltage or pulse width

// red to blue with cv
// green for gate?

// rotate is probably too weird
// have slider be more similar to macro mode...

/*
turning on and defaulting to sequencer mode and having it act the way it currently does seems like the most intuitive
way that this module could possibly work
*/

/* october 9th
when looking at sequencer outputs, pressing a scene button should select that channel.
channel settings button should be disabled

channel settings (shift + scene button)
global settings (shift)
dim color for channel mode off.
move random seq path to seq dir knob
random amount && random seed per channel value

macro:
shift == random should randomize nearest scened
shift + scene == per bank channel random amount

both: play button stops when playing and restarts when stopped
transpose: post quantizer

seq add button tap tempo?


pausing keeps fireing gate
also, divided gates keep firing.
gates not firing on external clock.

//transpose and range.

changing channels while locked on a page shouldnt reset the page
have pause stop where sequence is at and not reset.
maybe clipboards should reset eachother?

have holding multiple seq settings buttons change all pressed buttons together

make bpm "fine" work.

shift + add should delete along with normal add + shift

get rid of default clock divider and just have per channel ones.

have gates be activate above 50% instead of 1/65535!!!!!

quantize prior to morph??

per channel morph! work same as channel mode

internal clock class fix, add tap tempo

*/

volatile uint32_t time;

void main() {
	using namespace Catalyst2;

	// Force hardware test for now
	//
	// run_hardware_test();

	Params params;
	Ui::Interface ui{params};
	// UI ui{params};
	MacroSeq macroseq{params};

	mdrivlib::Timekeeper cv_stream(Board::cv_stream_conf, [&macroseq, &ui]() {
		auto time_now = HAL_GetTick();
		ui.Update();
		auto out = macroseq.Update();
		ui.SetOutputs(out);
		time = HAL_GetTick() - time_now;
	});

	ui.Start();
	cv_stream.start();
	while (true) {
		__NOP();
	}
}
