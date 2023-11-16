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

have the way phase offset is displayed be actual step instead of percentage

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
