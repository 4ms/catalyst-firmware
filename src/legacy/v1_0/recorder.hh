#pragma once

#include "conf/model.hh"
#include "util/math.hh"
#include <array>

namespace Catalyst2::Legacy::V1_0::Macro::Recorder
{

struct Data : public std::array<uint16_t, Model::Macro::rec_buffer_size> {
	uint32_t length{0};
	bool Validate() const {
		auto ret = true;
		for (auto &i : *this) {
			ret &= i <= 8190;
		}
		ret &= length < Model::Macro::rec_buffer_size;
		return ret;
	}
};

} // namespace Catalyst2::Legacy::V1_0::Macro::Recorder
