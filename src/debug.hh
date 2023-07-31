#pragma once
#include "conf/board_conf.hh"

namespace Debug
{
struct Disabled {
	static void high()
	{}
	static void low()
	{}
};

using Pin0 = mdrivlib::FPin<Board::Debug1.gpio, Board::Debug1.pin>;

}; // namespace Debug

#define GCC_OPTIMIZE_OFF __attribute__((optimize("-O0")))
