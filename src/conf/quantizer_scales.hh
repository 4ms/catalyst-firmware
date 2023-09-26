#pragma once
#include "../../lib/cpputil/util/colors.hh"
#include "../scales.hh"
#include "palette.hh"

namespace Catalyst2::Model
{

struct ScalePalette {
	Color color;
	Scale scale;
};

static constexpr ScalePalette off = {Palette::off, Scale{}};
static constexpr ScalePalette chromatic = {Palette::grey, Scales::chromatic};
static constexpr ScalePalette major = {Palette::green, Scales::major};
static constexpr ScalePalette minor = {Palette::red, Scales::minor};
static constexpr ScalePalette major_pentatonic = {Palette::orange, Scale{0.f, 2.f, 4.f, 7.f, 9.f}};
static constexpr ScalePalette minor_pentatonic = {Palette::purple, Scale{0.f, 3.f, 5.f, 7.f, 10.f}};
static constexpr ScalePalette wholetone = {Palette::cyan, Scale{0.f, 2.f, 4.f, 8.f, 10.f}};

// TODO: make sure Scales is not a name used elsewhere
static constexpr std::array Scales = {
	off,
	chromatic,
	major,
	minor,
	major_pentatonic,
	minor_pentatonic,
	wholetone,
};

} // namespace Catalyst2::Model
