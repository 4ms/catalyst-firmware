#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace Catalyst2
{

template<typename T, unsigned max_size>
class FixedFwList {
public:
	FixedFwList()
	{
		count = 0;
		previous = 0;
	}

	unsigned size() const
	{
		return count;
	}

	T read(unsigned index) const
	{
		if (count == 0)
			return 0; // hmm

		return data[index % count];
	}

	bool replace(unsigned index, T d)
	{
		if (count == 0)
			return false;

		data[index % count] = d;
		previous = index;
		return true;
	}

	bool insert(const T &d)
	{
		return insert(previous, d);
	}

	bool insert(unsigned index, T d)
	{
		if (count >= max_size || index > count)
			return false;

		if (count == 0 || count == 1) {
			previous = count;
			count += 1;
			return replace(previous, d);
		}

		if (count - index == 1) {
			data[count] = d;
			previous = count;
			count += 1;
			return true;
		}

		index += 1;

		std::move_backward(&data[index], &data[count], &data[count + 1]);

		data[index] = d;
		count += 1;
		previous = index;

		return true;
	}

	bool erase(unsigned index)
	{
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
		previous = 0;
		return true;
	}

private:
	std::array<T, max_size> data;
	unsigned previous;
	unsigned count;
};
} // namespace Catalyst2