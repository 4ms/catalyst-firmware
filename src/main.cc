#include "conf/board_conf.hh"
#include "controls.hh"
#include "debug.hh"
#include "drivers/timekeeper.hh"
#include "macro_seq.hh"
#include "outputs.hh"
#include "system.hh"

namespace
{
// Initialize the system before main()
Catalyst2::System _init;
} // namespace

void main()
{
	using namespace Catalyst2;

	Controls controls;
	Flags flags;
	Params params{controls, flags};
	MacroSeq macroseq{params, flags};
	Outputs outs;

	mdrivlib::Timekeeper cvstream(Board::cv_stream_conf, [&macroseq, &params, &outs]() {
		params.update();
		auto out = macroseq.update();
		outs.write(out);
	});

	params.start();
	cvstream.start();

	while (true) {
		__NOP();
	}
}
