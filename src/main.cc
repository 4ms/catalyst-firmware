#include "controls.hh"
#include "debug.hh"
#include "drivers/timekeeper.hh"
#include "macro_seq.hh"
#include "system.hh"
#include "ui.hh"

namespace
{
// Initialize the system before main()
Catalyst2::System _init;
} // namespace

void main()
{
	using namespace Catalyst2;

	Controls controls;
	Params params;
	UI ui{controls, params};
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
