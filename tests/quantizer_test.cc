#include "../src/quantizer.hh"

#include "doctest.h"
#include "util/countzip.hh"

using namespace Catalyst2;

TEST_CASE("Quantizer: number of transitions is correct") {
	// constexpr auto chromatic = QuantizerScale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f};
	Quantizer::Interface quan;

	// on init no scale will be loaded. check to make sure the quantizer doesnt affect the input value
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		CHECK(i == quan.Process(i));
	}

	constexpr auto tscale0 = Quantizer::Scale{0.f};
	quan.Load(tscale0);

	// this scale will be octaves only.
	auto output_values = 0u;
	auto prev_out_value = -1;
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		const auto temp = quan.Process(i);
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
	constexpr auto tscale1 = Quantizer::Scale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f};
	quan.Load(tscale1);

	output_values = 0u;
	prev_out_value = -1;
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		const auto temp = quan.Process(i);
		if (temp == prev_out_value)
			continue;
		prev_out_value = temp;
		output_values += 1;
	}

	expected_output_values = static_cast<unsigned int>(tscale1.size() * Model::output_octave_range);
	expected_output_values += 1;

	CHECK(output_values == expected_output_values);
}

TEST_CASE("Quantizer: picks closest note") {
	constexpr auto chromatic = Quantizer::Scale{0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f};
	Quantizer::Interface quan;
	quan.Load(chromatic);

	std::array<Channel::Cv::type, (size_t)Model::output_octave_range * 12> notes;
	for (auto [i, note] : enumerate(notes)) {
		note = i * Channel::Cv::inc_step;
	}
	CHECK((Channel::Cv::octave / 12) == Channel::Cv::inc_step);

	for (auto note : notes) {
		// Dead-on note value quantizes to itself
		CHECK(quan.Process(note) == note);

		// Slightly sharp should quantize back to the note
		CHECK(quan.Process(note + 1) == note);

		// Slightly flat should quantize back to the note
		if (note > 0)
			CHECK(quan.Process(note - 1) == note);

		// Very sharp should still quantize to closest
		CHECK(quan.Process(note + Channel::Cv::inc_step / 2) == note);

		// Too sharp and it quantizes to the next
		CHECK(quan.Process(note + Channel::Cv::inc_step / 2 + 1) == note + Channel::Cv::inc_step);

		// Very flat should still quantize to closest
		if (note >= Channel::Cv::inc_step / 2)
			CHECK(quan.Process(note - Channel::Cv::inc_step / 2) == note);

		// Too flat and it goes to the next note down
		if (note >= Channel::Cv::inc_step)
			CHECK(quan.Process(note - Channel::Cv::inc_step / 2 - 1) == note - Channel::Cv::inc_step);
	}
}
