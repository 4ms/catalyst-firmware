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
// do oneshot

// TODO: make color fades appear more linear

/* october 9th

maybe clipboards should reset eachother?

gate bright green needs fix

*/

/*
12/5

make sure you can adjust random values even if range technically shouldnt let you
val - clamp(val + inc, rangemin - random, rangemax - random)

*/
/*
12/19
bug when switch modes it will get stuck on bank mode leds or morph mode leds sometimes
make play led work
save tap tempo time on power off1
hide playhead when adjusting that step
make channel value 0 - 16384 instead of uint16 range so that it can go above and below the max without overflowing and
without using more data
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
