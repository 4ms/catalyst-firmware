#pragma once

#include <cstddef>
#include <cstdint>

namespace Catalyst2
{

class Flash {
public:
	// TODO
	static constexpr size_t cell_nr_ = 0u;
	struct data_t {
		bool validate() {
			return false;
		}
	};
	bool read(data_t data, size_t size) {
		return false;
	}
	bool write(data_t data, size_t size) {
		return false;
	}
	bool erase() {
		return false;
	}
	bool is_writeable(size_t) {
		return false;
	}
};

} // namespace Catalyst2