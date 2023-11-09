#pragma once

namespace Catalyst2::Debug
{

struct PinStub {
	static void high() {
	}
	static void low() {
	}
};

using Pin0 = PinStub;

}; // namespace Catalyst2::Debug

#define GCC_OPTIMIZE_OFF
