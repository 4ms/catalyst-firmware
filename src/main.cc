#include "app.hh"
#include "controls.hh"
#include "debug.hh"
#include "drivers/timekeeper.hh"
#include "hardware_tests/hardware_tests.hh"
#include "system.hh"
#include "ui.hh"

namespace
{
Catalyst2::System _init;
} // namespace

// maybe hold play to turn on loop?

// TODO: make color fades appear more linear

/* october 9th
maybe clipboards should reset eachother?
*/

// !paused = sequencer is moving, either from internal clock or external
// pause = sequence stops moving, playhead remains where it is
// reset = send sequence back to beginning regardless of play / pause
// stop = pause && reset && mute gate output

int main() {
	using namespace Catalyst2;

	run_hardware_test();

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
