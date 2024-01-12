#include "../src/sequencer.hh"

#include "doctest.h"

using namespace Catalyst2;

/*
floating point error test. sequencer crashes if (mode == ping pong && length == 1)

// bug: e8296861a9850b32b98ac56c21036c35b36c2f3a
// unit test passes: b3e5aacc1990699af8339e83406d95f9228d49a6
// hardware doesn't crash anymore ^^

*/

TEST_CASE("Pingpong length bug") {

	// Sequencer::Settings::Data settings;
	// Sequencer::PlayerInterface player{settings};

	// using enum Sequencer::Settings::PlayMode::Mode;

	// while (settings.GetPlayMode() != PingPong)
	//	settings.IncPlayMode(1);

	// CHECK(settings.GetPlayMode() == PingPong);

	// while (settings.GetLength() != Model::MinSeqSteps)
	//	settings.IncLength(-1);

	// CHECK(settings.GetLength() == Model::MinSeqSteps);

	// for (auto i = 0u; i < 1000u; i++) {
	//	player.Step();
	//	player.GetPlayheadStep(0);
	// }
}
