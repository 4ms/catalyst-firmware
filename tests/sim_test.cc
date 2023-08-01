#include "flags.hh"
#include "macro_seq.hh"
#include "outputs.hh"

struct Params {};

// Test if we could make a simulator
void sim_main()
{
	using namespace Catalyst2;
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
