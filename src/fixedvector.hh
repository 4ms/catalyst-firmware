#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

template<typename T, unsigned max_size>
class FixedVector {
	std::array<T, max_size> data;
	unsigned count{0};

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
	auto begin() const {
		return data.begin();
	}
	auto end() const {
		return begin() + count;
	}

	void insert(unsigned index, T d) {
		if (count >= max_size || index > count)
			return;

		if (index == count - 1) {
			data[count] = data[index];
			data[index] = d;
			count += 1;
			return;
		}

		if (index == count) {
			data[count] = d;
			count += 1;
			return;
		}

		index += 1;

		if (count != index) {
			std::move_backward(&data[index], &data[count], &data[count + 1]);
		}

		data[index] = d;
		count += 1;

		return;
	}

	void erase(const unsigned index) {
		if (count == 0 || index >= count)
			return;

		if (count == 1) {
			count = 0;
			return;
		}

		if (index == count - 1) {
			count -= 1;
			return;
		}

		std::move(&data[index + 1], &data[count], &data[index]);
		count -= 1;
		return;
	}
};
} // namespace Catalyst2