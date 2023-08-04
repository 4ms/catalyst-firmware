#pragma once
#include "conf/board_conf.hh"
#include "conf/model.hh"
#include "drivers/spi.hh"
#include "util/countzip.hh"
#include <span>

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

		uint8_t startup[3];
		startup[0] = start_command;
		startup[1] = 0;
		startup[2] = 0;

		send_data(startup);
	}

	void write(const Model::OutputBuffer &out)
	{
		// write to the DAC
		uint8_t data_buffer[3];

		for (auto [chan, val] : countzip(out)) {
			data_buffer[0] = write_command | chan;
			data_buffer[1] = static_cast<uint8_t>(val >> 8) ^ 0xff;
			data_buffer[2] = static_cast<uint8_t>(val & 0xff) ^ 0xff;
			send_data(data_buffer);
		}
	}

private:
	void send_data(std::span<uint8_t> dbuf)
	{
		spi.select<0>();

		for (auto dbyte : dbuf) {
			while (!spi.tx_space_available())
				;
			spi.load_tx_data(dbyte);
		}

		while (!spi.is_end_of_transfer())
			;
		spi.unselect<0>();
	}

	// write to and update dac channel N
	// datasheet page 20
	static constexpr uint8_t start_command = (1u << 6);
	static constexpr uint8_t write_command = (1u << 5) | (1u << 4);

	SpiPeriph<Board::DacSpiConf> spi;
};

} // namespace Catalyst2
