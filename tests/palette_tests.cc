#include "conf/palette.hh"
#include "doctest.h"

TEST_CASE("color") {

	using namespace Catalyst2;

	auto almost_full = Palette::Cv::fromLevel(0, 4499, Catalyst2::Channel::Cv::Range{});
	CHECK(almost_full.red() <= Palette::Voltage::Positive.red());
	CHECK(almost_full.green() <= Palette::Voltage::Positive.green());
	CHECK(almost_full.blue() <= Palette::Voltage::Positive.blue());

	auto full = Catalyst2::Palette::Cv::fromLevel(0, 4500, Catalyst2::Channel::Cv::Range{});
	CHECK(full.red() == Palette::Voltage::Positive.red());
	CHECK(full.green() == Palette::Voltage::Positive.green());
	CHECK(full.blue() == Palette::Voltage::Positive.blue());

	auto full_neg = Catalyst2::Palette::Cv::fromLevel(0, 0, Catalyst2::Channel::Cv::Range{});
	CHECK(full_neg.red() == Palette::Voltage::Negative.red());
	CHECK(full_neg.green() == Palette::Voltage::Negative.green());
	CHECK(full_neg.blue() == Palette::Voltage::Negative.blue());

	auto path_page_1_scene_1 = Catalyst2::Palette::Pathway::color(1.f / 64.f);
	auto path_page_1_scene_8 = Catalyst2::Palette::Pathway::color(8.f / 64.f);
	CHECK(path_page_1_scene_1 == path_page_1_scene_8);

	auto path_page_2_scene_1 = Catalyst2::Palette::Pathway::color(9.f / 64.f);
	auto path_page_2_scene_8 = Catalyst2::Palette::Pathway::color(16.f / 64.f);
	CHECK(path_page_2_scene_1 != path_page_1_scene_8);
	CHECK(path_page_2_scene_1 == path_page_2_scene_8);
}
