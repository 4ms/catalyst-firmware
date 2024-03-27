#pragma once

#include "channel.hh"
#include <bit>
#include <cstdint>

namespace Catalyst2::Macro
{
class Value {
	uint32_t cv : Channel::Cv::bits = Channel::Cv::zero;
	uint32_t gate : Channel::Gate::bits = 0;

public:
	void IncCv(int32_t inc, bool fine, Channel::Cv::Range range) {
		cv = Channel::Cv::Inc(cv, inc, fine, range);
	}
	void IncGate(int32_t inc) {
		gate = Channel::Gate::Inc(gate, inc);
	}
	Channel::Cv::type ReadCv(float random = 0.f) {
		return Channel::Cv::Read(cv, random);
	};
	float ReadGate(float random = 0.f) {
		return Channel::Gate::Read(gate, random);
	}
	bool Validate() const {
		auto ret = true;
		ret &= Channel::Cv::Validate(cv);
		ret &= Channel::Gate::Validate(gate);
		return ret;
	}
};
} // namespace Catalyst2::Macro
