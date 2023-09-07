#include "../src/quantizer.hh"
#include "../src/scales.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("Fixed Linked list")
{
	Quantizer<15> quantizer;
	quantizer.load_scale(Scales::chromatic);

	constexpr auto max_notes = Scales::chromatic.size() * 15;

	auto is_diff = -1;
	auto num_diffs = 0;
	for (int x = 0; x < 0xffff; x++) {

		auto temp = quantizer.process(x);
		if (temp != is_diff) {
			is_diff = temp;
			num_diffs += 1;
		}
	}

	CHECK(num_diffs == max_notes);
}
