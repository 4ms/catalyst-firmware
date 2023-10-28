#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

template<typename T, unsigned max_size>
class FixedVector {
public:
	const T &operator[](std::size_t idx) const {
		return data[idx];
	}
	T &operator[](std::size_t idx) {
		return data[idx];
	}
	std::size_t size() const {
		return count;
	}

	bool insert(unsigned index, const T d) {
		if (count >= max_size || index > count)
			return false;

		if (count == 0 || count == 1) {
			data[count] = d;
			count += 1;
			return true;
		}

		index += 1;

		if (count != index) {
			std::move_backward(&data[index], &data[count], &data[count + 1]);
		}

		data[index] = d;
		count += 1;

		return true;
	}

	bool erase(const unsigned index) {
		if (count == 0 || index >= count)
			return false;

		if (count == 1) {
			count = 0;
			return true;
		}

		if (index == count - 1) {
			count -= 1;
			return true;
		}

		std::move(&data[index + 1], &data[count], &data[index]);
		count -= 1;
		return true;
	}

private:
	std::array<T, max_size> data;
	unsigned count{0};
};
} // namespace Catalyst2