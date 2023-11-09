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

// skip or mute steps in seq

// have slider be more similar to macro mode...

/* october 9th

macro:
shift == random should randomize nearest scened
shift + scene == per bank channel random amount

both: play button stops when playing and restarts when stopped
transpose: post quantizer

//transpose and range.

maybe clipboards should reset eachother?

have holding multiple seq settings buttons change all pressed buttons together

have gates be activate above 50% instead of 1/65535!!!!!

quantize prior to morph??

ratchet options 1,2,3,4...
tap tempo should reset based on the tempo being tapped in.

have pause stop where sequence is at and not reset.

Transpose into range. t -> t+r

have the way phase offset is displayed be actual step instead of percentage

seq morph no jump. adjust slope to start at current max and end higher.

*/

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
