#include "../src/quantizer.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("Quantizer") {
	// constexpr auto chromatic = QuantizerScale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f};
	Quantizer::Interface quan;

	// on init no scale will be loaded. check to make sure the quantizer doesnt affect the input value
	for (auto i = Channel::min; i < Channel::max; i++) {
		CHECK(i == quan.Process(i));
	}

	constexpr auto tscale0 = Quantizer::Scale{0.f};
	quan.Load(tscale0);

	// this scale will be octaves only.
	auto output_values = 0u;
	auto prev_out_value = -1;
	for (auto i = Channel::min; i < Channel::max; i++) {
		const auto temp = quan.Process(i);
		if (temp == prev_out_value)
			continue;
		prev_out_value = temp;
		output_values += 1;
	}

	auto expected_output_values = static_cast<unsigned int>(tscale0.size() * Model::output_octave_range);

	CHECK(output_values == expected_output_values);

	// let's try a bigger scale
	constexpr auto tscale1 = Quantizer::Scale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f};
	quan.Load(tscale1);

	output_values = 0u;
	prev_out_value = -1;
	for (auto i = Channel::min; i < Channel::max; i++) {
		const auto temp = quan.Process(i);
		if (temp == prev_out_value)
			continue;
		prev_out_value = temp;
		output_values += 1;
	}

	expected_output_values = static_cast<unsigned int>(tscale1.size() * Model::output_octave_range);

	CHECK(output_values == expected_output_values);
}
