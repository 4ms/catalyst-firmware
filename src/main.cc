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

maybe clipboards should reset eachother?

have holding multiple seq settings buttons change all pressed buttons together

remove trig sense function to free a pin

remove mode switch
think of button combo to replace

tap tempo overrides external clock
external clock overrides tap tempo

gate bright green needs fix

gate doesnt fire in macro mode if pressing new button while another is already held down
p
no gate out when scrubbing

random setting color should be the same across channels

save on inactivity

do button led countdown before clearing

*/

/*
12/5

have save be press and hold bank + morph
and switch mode be hold

press and hold three buttons on either side during power on to boot into alternate mode and save that setting
same but already on, don't save

Clamp range in main app
make sure you can adjust random values even if range technically shouldnt let you
val - clamp(val + inc, rangemin - random, rangemax - random)

macro gets stuck on switch sometimes... bugbug
clear settings when clearing a single sequence channel


*/

void main() {
	using namespace Catalyst2;

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
