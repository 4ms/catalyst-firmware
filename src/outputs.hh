#pragma once
#include "conf/board_conf.hh"
#include "drivers/spi.hh"

namespace Catalyst2
{

class Outputs {

public:
	Outputs()
	{
		// sets up peripheral using pins/config from board_conf.hh
		// use Board::DacConf
	}

	void write(const Board::OutputBuffer &out)
	{
		//
	}
};

} // namespace Catalyst2
