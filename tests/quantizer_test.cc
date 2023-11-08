#include "../src/quantizer.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("Quantizer list" * doctest::skip(true)) {
	// constexpr auto chromatic = QuantizerScale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f};

	// constexpr auto octave_range = 15;
	// Quantizer<octave_range> quantizer;
	// quantizer.LoadScale(chromatic);

	// constexpr auto max_notes = chromatic.size() * octave_range;

	// auto is_diff = -1;
	// auto num_diffs = 0;
	// for (int x = 0; x < 0xffff; x++) {
	// 	auto temp = quantizer.Process(x);
	// 	if (temp != is_diff) {
	// 		is_diff = temp;
	// 		num_diffs += 1;
	// 	}
	// }

	// CHECK(num_diffs == max_notes);
}
