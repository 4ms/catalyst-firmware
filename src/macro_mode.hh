#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>

namespace Catalyst2::Macro::Mode
{
enum class Mode : uint8_t {
	BLIND,
	NORMAL,
	LATCH,
};
inline constexpr auto ModeMax = 3;

struct Data {
	Mode mode{Mode::NORMAL};

	bool Validate() const {
		return std::to_underlying(mode) < ModeMax;
	}
};

class Interface {
	Data &data;

public:
	Interface(Data &data)
		: data{data} {
	}

	bool operator==(Mode mode) {
		return data.mode == mode;
	}
	void Inc(int32_t inc) {
		auto t = std::to_underlying(data.mode);
		t = std::clamp<int32_t>(t + inc, 0, ModeMax - 1);
		data.mode = static_cast<Mode>(t);
	}
	void Clear() {
		data = Data{};
	}
};

} // namespace Catalyst2::Macro::Mode
