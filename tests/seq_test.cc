#include "../src/sequencer.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("Rotate steps") {

	Shared::Data shared_data;
	Shared::Interface shared{shared_data};
	Sequencer::Data data;
	Sequencer::Interface seq{data, shared};
	Channel::Range range{};

	// Fill a page with some values
	seq.slot.channel[0][0].IncCv(10, false, range);
	seq.slot.channel[0][1].IncCv(20, false, range);
	seq.slot.channel[0][2].IncCv(30, false, range);
	seq.slot.channel[0][3].IncCv(40, false, range);
	seq.slot.channel[0][4].IncCv(50, false, range);
	seq.slot.channel[0][5].IncCv(60, false, range);
	seq.slot.channel[0][6].IncCv(70, false, range);
	seq.slot.channel[0][7].IncCv(80, false, range);

	SUBCASE("Forwards") {
		auto original_vals = seq.GetPageValuesCv(0);
		seq.RotateStepsRight(0, 7);
		auto rotated_vals = seq.GetPageValuesCv(0);

		CHECK(rotated_vals[0] == original_vals[7]);
		CHECK(rotated_vals[1] == original_vals[0]);
		CHECK(rotated_vals[2] == original_vals[1]);
		CHECK(rotated_vals[3] == original_vals[2]);
		CHECK(rotated_vals[4] == original_vals[3]);
		CHECK(rotated_vals[5] == original_vals[4]);
		CHECK(rotated_vals[6] == original_vals[5]);
		CHECK(rotated_vals[7] == original_vals[6]);
	}

	SUBCASE("Backwards") {
		auto original_vals = seq.GetPageValuesCv(0);
		seq.RotateStepsLeft(0, 7);
		auto rotated_vals = seq.GetPageValuesCv(0);

		CHECK(original_vals[0] == rotated_vals[7]);
		CHECK(original_vals[1] == rotated_vals[0]);
		CHECK(original_vals[2] == rotated_vals[1]);
		CHECK(original_vals[3] == rotated_vals[2]);
		CHECK(original_vals[4] == rotated_vals[3]);
		CHECK(original_vals[5] == rotated_vals[4]);
		CHECK(original_vals[6] == rotated_vals[5]);
		CHECK(original_vals[7] == rotated_vals[6]);
	}
}
