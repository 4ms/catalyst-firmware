#pragma once

#include <array>

namespace Catalyst2
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
	constexpr std::size_t size() const
	{
		return size_;
	}
	constexpr auto begin() const
	{
		return scl.begin();
	}
	constexpr auto end() const
	{
		return begin() + size_;
	}

private:
	std::array<float, 12> scl;
	std::size_t size_;
};

} // namespace Catalyst2
