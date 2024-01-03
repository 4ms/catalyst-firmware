#pragma once

#include "bootloader/animation.hh"
#include "bootloader/bootloader-controls.hh"
#include "conf/bootloader_settings.hh"
#include "conf/flash_layout.hh"
#include "drivers/flash.hh"
#include "drivers/stm32xx.h"
#include "system.hh"
#include "util/analyzed_signal.hh"
#include "util/zip.hh"
#include <cstring> //for memcpy: adds 100Bytes to binary vs. using our own

#define USING_FSK
// #define USING_QPSK

#ifdef USING_QPSK
#include "stm_audio_bootloader/qpsk/demodulator.h"
#include "stm_audio_bootloader/qpsk/packet_decoder.h"
#else
#include "stm_audio_bootloader/fsk/demodulator.h"
#include "stm_audio_bootloader/fsk/packet_decoder.h"
#endif

namespace Catalyst2::Bootloader
{

struct GateBootloader {
#ifdef USING_QPSK
	static constexpr float kModulationRate = 6000.0;
	static constexpr float kBitRate = 12000.0;
	static constexpr float kSampleRate = 48000.0;
#else
	static constexpr uint32_t kSampleRate = BootloaderConf::SampleRate;		 //-s
	static constexpr uint32_t kPausePeriod = BootloaderConf::Encoding.blank; //-b
	static constexpr uint32_t kOnePeriod = BootloaderConf::Encoding.one;	 //-n
	static constexpr uint32_t kZeroPeriod = BootloaderConf::Encoding.zero;	 //-z
#endif
	static constexpr uint32_t kStartReceiveAddress = BootloaderReceiveAddr;
	static constexpr uint32_t kBlkSize = BootloaderConf::ReceiveSectorSize;					   // Flash page size, -g
	static constexpr uint16_t kPacketsPerBlock = kBlkSize / stm_audio_bootloader::kPacketSize; // kPacketSize=256

	uint8_t recv_buffer[kBlkSize];

	stm_audio_bootloader::PacketDecoder decoder;
	stm_audio_bootloader::Demodulator demodulator;

	uint16_t packet_index = 0;
	uint16_t discard_samples = 16000;
	uint32_t current_flash_address = 0;

	enum class UiState { WAITING, RECEIVING, ERROR, WRITING, DONE };
	UiState ui_state{};

	PeakMeter<int32_t> meter;

	Controls controls;
	LedAnimation leds{controls};

	GateBootloader() {
		// init_buttons();
		leds.animate(Animation::RESET);
	}

	bool check_enter_bootloader() {
		HAL_Delay(300);

		uint32_t dly = 32000;
		uint32_t button_debounce = 0;
		while (dly--) {
			if (controls.button.shift.is_pressed() && controls.button.fine.is_pressed())
				button_debounce++;
			else
				button_debounce = 0;
		}
		HAL_Delay(100);

		while (controls.button.shift.is_pressed() && controls.button.fine.is_pressed())
			;

		bool do_bootloader = (button_debounce > 15000);
		return do_bootloader;
	}

	// GCC_OPTIMIZE_OFF
	void run() {
		init_reception();

		mdrivlib::Timekeeper update_task{
			{
				.TIMx = TIM3,
				.period_ns = 1'000'000'000 / kSampleRate,
				.priority1 = 0,
				.priority2 = 0,
			},
			[this] {
				if (!discard_samples) {
					controls.clk_in.update();
					bool sample = controls.clk_in.is_high();
					demodulator.PushSample(sample);
				} else
					--discard_samples;
			},
		};
		update_task.start();

		uint32_t button_exit_armed = 0;
		uint32_t rev_but_armed = 0;

		HAL_Delay(300);

		uint8_t exit_updater = false;
		while (!exit_updater) {
			bool rcv_err = false;

			while (demodulator.available() && !rcv_err && !exit_updater) {
				uint8_t symbol = demodulator.NextSymbol();
				auto state = decoder.ProcessSymbol(symbol);

				switch (state) {
					case stm_audio_bootloader::PACKET_DECODER_STATE_SYNCING:
						leds.animate(Animation::SYNC);
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_OK:
						ui_state = UiState::RECEIVING;
						memcpy(recv_buffer + (packet_index % kPacketsPerBlock) * stm_audio_bootloader::kPacketSize,
							   decoder.packet_data(),
							   stm_audio_bootloader::kPacketSize);
						++packet_index;
						if ((packet_index % kPacketsPerBlock) == 0) {
							ui_state = UiState::WRITING;
							bool write_ok = write_buffer();
							if (!write_ok) {
								ui_state = UiState::ERROR;
								rcv_err = true;
							}
							new_block();
						} else {
							new_packet();
						}
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_ERROR_SYNC:
						rcv_err = true;
						// Console::write("Sync Error\n");
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_ERROR_CRC:
						rcv_err = true;
						// Console::write("CRC Error\n");
						break;

					case stm_audio_bootloader::PACKET_DECODER_STATE_END_OF_TRANSMISSION:
						// Console::write("End\n");
						// Write out buffer if we haven't yet (which happens if we load only to RAM)
						if (current_flash_address == kStartReceiveAddress) {
							if (!write_buffer()) {
								ui_state = UiState::ERROR;
								rcv_err = true;
								// Console::write("Buffer Write Error\n");
								new_block();
								break;
							}
						}
						exit_updater = true;
						ui_state = UiState::DONE;
						copy_firmware();
						// Console::write("Success!\n");
						animate_until_button_pushed(Animation::SUCCESS, controls.button.play);
						leds.animate(Animation::RESET);
						HAL_Delay(100);
						break;

					default:
						break;
				}
			}

			if (rcv_err) {
				ui_state = UiState::ERROR;
				animate_until_button_pushed(Animation::FAIL_ERR, controls.button.play);
				leds.animate(Animation::RESET);
				HAL_Delay(100);
				init_reception();
				exit_updater = false;
			}

			if (controls.button.add.is_pressed()) {
				if (rev_but_armed) {
					HAL_Delay(100);
					init_reception();
				}
				rev_but_armed = 0;
			} else
				rev_but_armed = 1;

			if (controls.button.play.is_pressed()) {
				if (button_exit_armed) {
					if (ui_state == UiState::WAITING) {
						exit_updater = true;
					}
				}
				button_exit_armed = 0;
			} else
				button_exit_armed = 1;
		}

		update_task.stop();
		animate_task.stop();

		ui_state = UiState::DONE;

		while (controls.button.play.is_pressed() || controls.button.add.is_pressed())
			;
	}

	void init_reception() {
#ifdef USING_QPSK
		// QPSK
		decoder.Init((uint16_t)20000);
		demodulator.Init(
			kModulationRate / kSampleRate * 4294967296.0f, kSampleRate / kModulationRate, 2.f * kSampleRate / kBitRate);
		demodulator.SyncCarrier(true);
		decoder.Reset();
#else
		// FSK
		decoder.Init();
		decoder.Reset();
		demodulator.Init(kPausePeriod, kOnePeriod, kZeroPeriod); // pause_thresh = 24. one_thresh = 6.
		demodulator.Sync();
#endif
		current_flash_address = kStartReceiveAddress;
		packet_index = 0;
		ui_state = UiState::WAITING;
	}

	bool write_buffer() {
		// Console::write("Writing\n");
		if ((current_flash_address + kBlkSize) <= get_sector_addr(NumFlashSectors)) {
			auto data = std::span<uint32_t>{(uint32_t *)recv_buffer, kBlkSize / 4};
			mdrivlib::InternalFlash::write(data, current_flash_address);
			current_flash_address += kBlkSize;
			return true;
		} else {
			return false;
		}
	}

	void copy_firmware() {
		if (kStartReceiveAddress != AppFlashAddr) {
			// Console::write("Copying from receive sectors to execution sectors\n");
			uint32_t src_addr = kStartReceiveAddress;
			uint32_t dst_addr = AppFlashAddr;
			while (dst_addr < kStartReceiveAddress) {
				auto data = std::span<uint32_t>{(uint32_t *)src_addr, 16 * 1024};
				mdrivlib::InternalFlash::write(data, dst_addr);
				src_addr += 16 * 1024;
				dst_addr += 16 * 1024;
			}
		}
	}

	void update_LEDs() {
		if (ui_state == UiState::RECEIVING)
			leds.animate(Animation::RECEIVING);

		else if (ui_state == UiState::WRITING)
			leds.animate(Animation::WRITING);

		else if (ui_state == UiState::WAITING)
			leds.animate(Animation::WAITING);

		else // if (ui_state == UiState::DONE)
		{}
	}

	void new_block() {
		decoder.Reset();
#ifdef USING_FSK
		demodulator.Sync(); // FSK
#else
		demodulator.SyncCarrier(false); // QPSK
#endif
	}

	void new_packet() {
#ifdef USING_FSK
		decoder.Reset(); // FSK
#else
		decoder.Reset();
		demodulator.SyncDecision(); // QPSK
#endif
	}

	void animate_until_button_pushed(Animation animation_type, MuxedButton &button) {
		leds.animate(Animation::RESET);

		while (!button.is_pressed()) {
			HAL_Delay(1);
			leds.animate(animation_type);
		}
		while (button.is_pressed()) {
			HAL_Delay(1);
		}
	}
};

} // namespace Catalyst2::Bootloader
