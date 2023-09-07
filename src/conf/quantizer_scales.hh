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

static constexpr std::array Scales = {
	off,
	chromatic,
	major,
	minor,
};

} // namespace Catalyst2::Model
