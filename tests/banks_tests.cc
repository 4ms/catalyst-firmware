#include "../src/scene.hh"
#include "doctest.h"

TEST_CASE("Overflow with ints and unsigned")
{
	// constexpr Catalyst2::ChannelValue::type Max = Catalyst2::ChannelValue::Max;
	// constexpr Catalyst2::ChannelValue::type Min = Catalyst2::ChannelValue::Min;
	// int32_t by = -1;

	// Catalyst2::Banks banks;

	// banks.set_chan(0, 0, Catalyst2::ChannelValue::Max);
	// CHECK(banks.get_chan(0, 0) == Catalyst2::ChannelValue::Max);

	// banks.adjust_chan(0, 0, by);
	// CHECK(banks.get_chan(0, 0) == Catalyst2::ChannelValue::Max - 1);
	// banks.adjust_chan(0, 0, -1);
	// CHECK(banks.get_chan(0, 0) == Catalyst2::ChannelValue::Max - 2);

	// banks.adjust_chan(0, 0, 3);
	// CHECK(banks.get_chan(0, 0) == Catalyst2::ChannelValue::Max);

	// regarding the next two checks
	/* if ChannelValue::type == uint32_t then incrementing chan by "Max - Min" or "Min - Max" would be undefined right?
		max - min == UINT32_MAX and the adjust_chan func wants an int32
	*/
	// banks.set_chan(0, 0, Min);
	// banks.adjust_chan(0, 0, Max - Min);
	// CHECK(banks.get_chan(0, 0) == Max);

	// banks.set_chan(0, 0, Max);
	// banks.adjust_chan(0, 0, Min - Max);
	// CHECK(banks.get_chan(0, 0) == Min);

	// banks.set_chan(1, 1, Catalyst2::ChannelValue::Min);
	// CHECK(banks.get_chan(1, 1) == Catalyst2::ChannelValue::Min);
	// banks.adjust_chan(1, 1, -1);
	// CHECK(banks.get_chan(1, 1) == Catalyst2::ChannelValue::Min);
	// banks.adjust_chan(1, 1, 1);
	// CHECK(banks.get_chan(1, 1) == Catalyst2::ChannelValue::Min + 1);
	// banks.adjust_chan(1, 1, -2);
	// CHECK(banks.get_chan(1, 1) == Catalyst2::ChannelValue::Min);
}
