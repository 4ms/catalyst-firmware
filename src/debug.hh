#pragma once
#include "conf/board_conf.hh"

namespace Catalyst2::Debug
{
using Pin0 = Board::DebugPin;
}; // namespace Catalyst2::Debug

#define GCC_OPTIMIZE_OFF __attribute__((optimize("-O0")))
