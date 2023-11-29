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

// maybe hold play to turn on loop?
// do oneshot

// TODO: make color fades appear more linear

// better way to delete banks

/* october 9th

macro:
shift == random should randomize nearest scened
shift + scene == per bank channel random amount

both: play button stops when playing and restarts when stopped

//transpose and range.

maybe clipboards should reset eachother?

have holding multiple seq settings buttons change all pressed buttons together

quantize prior to morph??

Transpose into range. t -> t+r

TODO: phase offset doesnt display correctly in backwards and pingpong mode

*/

/*
remove trig sense function to free a pin

remove mode switch
think of button combo to replace

tap tempo overrides external clock
external clock overrides tap tempo

backwards is broken again!!!

morph fix seems to have offset the sequencer :(

gate bright green needs fix

gate doesnt fire in macro mode if pressing new button while another is already held down

no gate out when scrubbing

change red setting to dim grey

change channel setting color to not be sequencer head color

random setting color should be the same across channels

save on inactivity

delete on shift hold reset or shift hold delete in seq or macro respectively
do button led countdown before clearing

*/

void main() {
	using namespace Catalyst2;

	// run_hardware_test();

	Params params;
	Ui::Interface ui{params};
	MacroSeq macroseq{params};

	// 7% at 1kHz, 21% at 3kHz
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
