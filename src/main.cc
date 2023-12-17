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
