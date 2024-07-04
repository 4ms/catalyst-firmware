#include "../src/quantizer.hh"

#include "doctest.h"
#include "util/countzip.hh"
#include <set>
#include <span>

using namespace Catalyst2;

TEST_CASE("Quantizer: number of transitions is correct") {
	// on init no scale will be loaded. check to make sure the Quantizer::izer doesnt affect the input value
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		CHECK(i == Quantizer::Process(Quantizer::Scale{}, i));
	}

	constexpr auto tscale0 = Quantizer::Scale{12.f, 0.f};

	// this scale will be octaves only.
	auto output_values = 0u;
	auto prev_out_value = -1;
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		const auto temp = Quantizer::Process(tscale0, i);
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
	constexpr auto tscale1 = Quantizer::Scale{12.f, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f};

	output_values = 0u;
	prev_out_value = -1;
	for (auto i = Channel::Cv::min; i < Channel::Cv::max; i++) {
		const auto temp = Quantizer::Process(tscale1, i);
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

	std::array<Channel::Cv::type, (size_t)Model::output_octave_range * 12 / 2> notes;
	for (auto [i, note] : enumerate(notes)) {
		note = i * step_size;
	}

	for (auto note : notes) {
		// Dead-on note value Quantizer::izes to itself
		CHECK(Quantizer::Process(scale, note) == note);

		// Slightly flat should Quantizer::ize back to the note
		if (note > 0)
			CHECK(Quantizer::Process(scale, note - 1) == note);

		// Very flat should still Quantizer::ize to itself
		if (note >= step_size / 2)
			CHECK(Quantizer::Process(scale, note - (step_size / 2) + 1) == note);

		// Too flat and it goes to the next note down
		if (note >= step_size / 2)
			CHECK(Quantizer::Process(scale, note - (step_size / 2) - 1) == (note - step_size));

		// Slightly sharp should Quantizer::ize back to the note
		CHECK(Quantizer::Process(scale, note + 1) == note);

		// Very sharp should still Quantizer::ize to itself
		CHECK(Quantizer::Process(scale, note + (step_size / 2) - 1) == note);

		// Too sharp and it Quantizer::izes to the next
		CHECK(Quantizer::Process(scale, note + (step_size / 2) + 1) == (note + step_size));
	}
}

TEST_CASE("Quantizer: picks closest note (chromatic)") {
	constexpr auto chromatic = Quantizer::Scale{12.f, 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f};
	constexpr auto wholetones = Quantizer::Scale{12.f, 0.f, 2.f, 4.f, 6.f, 8.f, 10.f};

	check_scale(chromatic, Channel::Cv::note);
	check_scale(wholetones, Channel::Cv::note * 2);
}

TEST_CASE("Create scale from sequence of notes") {

	SUBCASE("100 300 600 + 1000*N") {
		FixedVector<Channel::Cv::type, Quantizer::Scale::MaxScaleNotes> seq;

		SUBCASE("notes in order, two octaves up") {
			seq.push_back(2100);
			seq.push_back(2300);
			seq.push_back(2600);
			seq.push_back(3100);
		}

		SUBCASE("notes out of order, with repeats") {
			seq.push_back(1100);
			seq.push_back(300);
			seq.push_back(600);
			seq.push_back(600);
			seq.push_back(1100);
			seq.push_back(100);
		}

		Quantizer::Scale scale{seq}; // construct scale from sequence
		CHECK(scale.octave == 1000);
		CHECK(scale.size() == 3);
		CHECK(scale[0] == 100);
		CHECK(scale[1] == 300);
		CHECK(scale[2] == 600);
	}

	SUBCASE("D F + 5th * N, crosses an octave when normalized to 0") {
		enum note : Channel::Cv::type { C, Cx, D, Dx, E, F, Fx, G, Gx, A, Ax, B };
		auto n = [](note n, unsigned octave) -> Channel::Cv::type { return 12u * octave + n; };

		FixedVector<Channel::Cv::type, Quantizer::Scale::MaxScaleNotes> seq;

		CHECK(n(D, 2) == 26);
		CHECK(n(F, 2) == 29); // 1
		CHECK(n(A, 2) == 33); // 5
		// Scale span is 7 (high-low)
		// So scale "octaves" are 0-6, 7-13, 14-20, 21-27, 28-34, ...
		// The notes 26, 29, 33 are in two different "octaves".
		// 26 is one "octave" below 33, so we can drop it and use 29, 33
		// for our scale definition.
		// Thus, the lowest note >= 0 is 1 (29-28), then 5 (33-28), repeating +N*7

		seq.push_back(n(D, 2));
		seq.push_back(n(F, 2));
		seq.push_back(n(A, 2));

		Quantizer::Scale scale{seq}; // construct scale from sequence

		auto check_is_in = [&](auto val) { CHECK(Quantizer::Process(scale, val) == val); };
		auto check_is_out = [&](auto val) { CHECK(Quantizer::Process(scale, val) != val); };

		// original values are in the scale:
		check_is_in(n(D, 2));
		check_is_in(n(F, 2));
		check_is_in(n(A, 2));

		// repeat this by adding fifths:
		check_is_in(n(C, 3));
		check_is_in(n(G, 3));
		check_is_in(n(B, 3));
		check_is_in(n(D, 4));
		check_is_in(n(Fx, 4));
		check_is_in(n(A, 4));
		check_is_in(n(Cx, 5));
		check_is_in(n(E, 5));
		check_is_in(n(Gx, 5));

		// make sure bogus values aren't also in scale (just check a few obvious ones)
		check_is_out(n(D, 5));
		check_is_out(n(F, 5));
		check_is_out(n(A, 5));

		// check going down from original notes, too
		check_is_in(n(Ax, 1));
		check_is_in(n(G, 1));
		check_is_in(n(Dx, 1));
		check_is_in(n(C, 1));
		check_is_in(n(Gx, 0));
		check_is_in(n(F, 0));
		check_is_in(n(Cx, 0));
	}
}
