#pragma once

#include <array>

namespace Scales
{

struct Scale {
	template<typename... T>
	constexpr Scale(T... ts)
		: scl{ts...}
		, size_(sizeof...(T))
	{}
	constexpr const float &operator[](const std::size_t idx) const
	{
		return scl[idx];
	}
	constexpr std::size_t size()
	{
		return size_;
	}

private:
	std::array<float, 12> scl;
	std::size_t size_;
};

static constexpr Scale chromatic = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f, 11.f};
static constexpr Scale major = {0.f, 2.f, 4.f, 5.f, 7.f, 9.f, 11.f};
static constexpr Scale minor = {0.f, 2.f, 3.f, 5.f, 7.f, 8.f, 10.f};

} // namespace Scales