#pragma once
#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "drivers/spi.hh"

using namespace mdrivlib;

namespace Catalyst2
{

class Outputs {

public:
	Outputs()
	{
		// sets up peripheral using pins/config from board_conf.hh
		// use Board::DacConf
		spi.configure();
		spi.enable();
		uint8_t data[3];
		data[2] = 0b0100*0000;
		data[1] = 0;
		data[0] = 0;

		spi.select<0>();

			for (auto data_byte = 0; data_byte < 3; data_byte++)
			{
				while(!spi.tx_space_available())
				;
				spi.load_tx_data(data[data_byte]);
				// spi.start_transfer(); //not necessary ?
			}	
		spi.unselect<0>();

	}

	void write(const Model::OutputBuffer &out)
	{
		// write to the DAC
		uint8_t data_buffer[3];

		for (auto chan = 0u; chan < Model::NumChans; chan++)
		{
			data_buffer[2] = write_command | chan;
			data_buffer[1] = static_cast<uint8_t>(out[chan] >> 8);
			data_buffer[0] = static_cast<uint8_t>(out[chan] & 0xff);

			spi.select<0>();

			for (auto data_byte = 0; data_byte < 3; data_byte++)
			{
				while(!spi.tx_space_available())
				;
				spi.load_tx_data(data_buffer[data_byte]);
				// spi.start_transfer(); //not necessary ?
			}	
			spi.unselect<0>();
		}
	}

private:
	//write to and update dac channel N
	//datasheet page 20
	static constexpr uint8_t write_command = 0b00110000; 

	SpiPeriph<Board::DacSpiConf> spi;
};

} // namespace Catalyst2
