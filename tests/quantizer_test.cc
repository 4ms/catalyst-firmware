#include "../src/quantizer.hh"

#include "doctest.h"
#include "util/countzip.hh"

using namespace Catalyst2;

TEST_CASE("Quantizer: number of transitions is correct") {
	Quantizer::Interface quan;

	// on init no scale will be loaded. check to make sure the quantizer doesnt affect the input value
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		CHECK(i == quan.Process(Quantizer::Scale{}, i));
	}

	constexpr auto tscale0 = Quantizer::Scale{12.f};

	// this scale will be octaves only.
	auto output_values = 0u;
	auto prev_out_value = -1;
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		const auto temp = quan.Process(tscale0, i);
		if (temp == prev_out_value)
			continue;
		prev_out_value = temp;
		output_values += 1;
	}

	// octave range is 15, so that means we have 16 possible outputs which are octaves:
	// 0*300, 1*300, 2*300, ... 15*300 = 16 values
	// So we have to do +1
	auto expected_output_values = static_cast<unsigned int>(tscale0.size() * Model::output_octave_range);
	expected_output_values += 1;

	CHECK(output_values == expected_output_values);

	// let's try a bigger scale
	constexpr auto tscale1 = Quantizer::Scale{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 12.f};

	output_values = 0u;
	prev_out_value = -1;
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		const auto temp = quan.Process(tscale1, i);
		if (temp == prev_out_value)
			continue;
		prev_out_value = temp;
		output_values += 1;
	}

	expected_output_values = static_cast<unsigned int>(tscale1.size() * Model::output_octave_range);
	expected_output_values += 1;

	CHECK(output_values == expected_output_values);
}

void check_scale(Quantizer::Scale const &scale, unsigned step_size) {
	Quantizer::Interface quan;

	std::array<Channel::Cv::type, (size_t)Model::output_octave_range * 12 / 2> notes;
	for (auto [i, note] : enumerate(notes)) {
		note = i * step_size;
	}

	for (auto note : notes) {
		// Dead-on note value quantizes to itself
		CHECK(quan.Process(scale, note) == note);

		// Slightly flat should quantize back to the note
		if (note > 0)
			CHECK(quan.Process(scale, note - 1) == note);

		// Very flat should still quantize to itself
		if (note >= step_size / 2)
			CHECK(quan.Process(scale, note - (step_size / 2) + 1) == note);

		// Too flat and it goes to the next note down
		if (note >= step_size / 2)
			CHECK(quan.Process(scale, note - (step_size / 2) - 1) == (note - step_size));

		// Slightly sharp should quantize back to the note
		CHECK(quan.Process(scale, note + 1) == note);

		// Very sharp should still quantize to itself
		CHECK(quan.Process(scale, note + (step_size / 2) - 1) == note);

		// Too sharp and it quantizes to the next
		CHECK(quan.Process(scale, note + (step_size / 2) + 1) == (note + step_size));
	}
}

TEST_CASE("Quantizer: picks closest note (chromatic)") {
	constexpr auto chromatic = Quantizer::Scale{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f, 12.f};
	constexpr auto wholetones = Quantizer::Scale{2.f, 4.f, 6.f, 8.f, 10.f, 12.f};

	check_scale(chromatic, Channel::Cv::note);
	check_scale(wholetones, Channel::Cv::note * 2);
}
