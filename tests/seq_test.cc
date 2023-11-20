#include "../src/randompool.hh"
#include "../src/sequencer.hh"

#include "doctest.h"

using namespace Catalyst2;

/*
floating point error test. sequencer crashes if mode is ping pong and length == 1

// bug at e8296861a9850b32b98ac56c21036c35b36c2f3a

*/

TEST_CASE("Pingpong length bug") {

	Sequencer::Data data;
	RandomPool dummy;
	Sequencer::Interface seq{data, dummy};

	while (data.global.playmode.Read().value() != Sequencer::PlayMode::PingPong)
		data.global.playmode.Inc(1);

	CHECK(data.global.playmode.Read().value() == Sequencer::PlayMode::PingPong);

	while (data.global.length.Read().value() != Model::MinSeqSteps)
		data.global.length.Inc(-1);

	CHECK(data.global.length.Read().value() == 1);

	for (auto i = 0u; i < 1000u; i++) {
		seq.player.Step();
		seq.player.GetPlayheadStep(0);
	}
}
