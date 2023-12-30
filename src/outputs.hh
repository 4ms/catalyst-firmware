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
	Outputs() {
		spi.configure();
		spi.enable();

		uint8_t startup[3];
		startup[0] = TriggerRegister::Address;
		startup[1] = 0;
		startup[2] = TriggerRegister::Reset;
		send_data(startup);

		HAL_Delay(2);

		startup[0] = GainRegister::Address;
		startup[1] = GainRegister::DisableRefDiv;
		startup[2] = GainRegister::AllDacsGainX2;
		send_data(startup);
	}

	void write(const Model::Output::Buffer &out) {
		uint8_t data_buffer[3];

		for (auto [chan, val] : countzip(out)) {
			data_buffer[0] = DacOutRegister::Address | chan;
			data_buffer[1] = static_cast<uint8_t>(val >> 8) ^ 0xff;
			data_buffer[2] = static_cast<uint8_t>(val & 0xff) ^ 0xff;
			send_data(data_buffer);
		}
	}

private:
	void send_data(std::span<uint8_t> dbuf) {
		spi.select<0>();

		for (auto dbyte : dbuf) {
			while (!spi.tx_space_available())
				;
			spi.load_tx_data(dbyte);
		}

		// See note in Reference Manual (sec 20.3.5)
		// During discontinuous communications, there is a 2 APB clock period delay between the write operation to
		// SPI_DR and the BSY bit setting. As a consequence, in transmit-only mode, it is mandatory to wait first until
		// TXE is set and then until BSY is cleared after writing the last data.
		while (!spi.tx_space_available())
			;
		while (!spi.is_end_of_transfer())
			;
		spi.unselect<0>();
	}

	struct GainRegister {
		static constexpr uint8_t Address = 0x04;
		static constexpr uint8_t DisableRefDiv = 0x00;
		static constexpr uint8_t AllDacsGainX2 = 0xFF;
	};

	struct DacOutRegister {
		static constexpr uint8_t Address = 0x08;
	};

	struct TriggerRegister {
		static constexpr uint8_t Address = 0x05;
		static constexpr uint8_t Reset = 0b1010;
	};

	SpiPeriph<Board::DacSpiConf> spi;
};

} // namespace Catalyst2
