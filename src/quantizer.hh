#pragma once

#include "scales.hh"
#include <array>
#include <span>

namespace Catalyst2
{

struct Quantizer {

	template<std::size_t S>
	constexpr Quantizer(const std::array<Scales::Scale, S> &arr)
		: num_scales(arr.size())
	{}

	// private:
	const std::size_t num_scales;
};

} // namespace Catalyst2