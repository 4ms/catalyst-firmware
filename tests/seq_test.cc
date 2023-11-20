#include "../src/randompool.hh"
#include "../src/sequencer.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("Pingpong length bug") {

	Sequencer::Data data;
	RandomPool dummy;
	Sequencer::Interface seq{data, dummy};

	data.global.playmode.Inc(1);
	data.global.playmode.Inc(1);

	CHECK(data.global.playmode.Read().value() == Sequencer::PlayMode::PingPong);

	data.global.length.Inc(-1);
	data.global.length.Inc(-1);
	data.global.length.Inc(-1);
	data.global.length.Inc(-1);
	data.global.length.Inc(-1);
	data.global.length.Inc(-1);
	data.global.length.Inc(-1);

	CHECK(data.global.length.Read().value() == 1);

	for (auto i = 0u; i < 1000; i++) {
		seq.player.Step();
		seq.player.GetPlayheadStep(0);
	}

	CHECK(true);
}
