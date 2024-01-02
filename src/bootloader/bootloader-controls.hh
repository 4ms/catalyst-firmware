#pragma once
#include "conf/board_conf.hh"
#include "muxed_button.hh"

namespace Catalyst2::Bootloader
{

struct Controls {

	struct Buttons {
		std::array<MuxedButton, Model::NumChans> scene{
			Board::Buttons::SceneMap[0],
			Board::Buttons::SceneMap[1],
			Board::Buttons::SceneMap[2],
			Board::Buttons::SceneMap[3],
			Board::Buttons::SceneMap[4],
			Board::Buttons::SceneMap[5],
			Board::Buttons::SceneMap[6],
			Board::Buttons::SceneMap[7],
		};
		MuxedButton shift{Board::Buttons::Shift};
		MuxedButton morph{Board::Buttons::Morph};
		MuxedButton bank{Board::Buttons::Bank};
		MuxedButton fine{Board::Buttons::Fine};
		MuxedButton add{Board::Buttons::Add};
		MuxedButton play{Board::Buttons::Play};
	};

	MuxedIO<Board::MuxConf> muxio;
	mdrivlib::Timekeeper muxio_update_task;
	std::array<uint8_t, Model::NumChans> button_led_duty;

	Board::PlayLed playled;
	Buttons button;

public:
	Board::TrigJack clk_in;

	Controls() {
		// 4.6%: 16kHz
		muxio_update_task.init(Board::muxio_conf, [this]() { UpdateMuxio(); });
		muxio_update_task.start();
	}

	void SetButtonLed(unsigned led, float intensity) {
		static constexpr std::array<uint8_t, 32> lut = {0, 0, 1, 1, 1, 1,  1,  1,  1,  2,  2,  2,  3,  3,  4,  4,
														5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 23, 25, 28, 32};
		intensity = std::clamp(0.f, 1.f, intensity);
		button_led_duty[led] = lut[intensity * (lut.size() - 1)];
	}

	void SetButtonLed(unsigned led, bool on) {
		button_led_duty[led] = on ? 32 : 0;
	}

	void SetPlayLed(bool on) {
		playled.set(on);
	}

private:
	void UpdateMuxio() {
		static uint8_t cnt = 0;
		auto button_leds = 0u;
		for (auto [led, bitshift] : zip(button_led_duty, Board::ButtonLedMap)) {
			if (cnt < led)
				button_leds |= 1ul << bitshift;
		}

		auto mux_read = muxio.step(button_leds);
		if (mux_read.has_value()) {
			UpdateButtons(mux_read.value());
			cnt++;
			if (cnt >= 32) {
				cnt = 0;
				UpdateButtons(mux_read.value());
			}
		}
	}

	void UpdateButtons(uint32_t raw_mux_read) {
		for (auto &but : button.scene) {
			but.update(raw_mux_read);
		}
		button.shift.update(raw_mux_read);
		button.morph.update(raw_mux_read);
		button.bank.update(raw_mux_read);
		button.fine.update(raw_mux_read);
		button.add.update(raw_mux_read);
		button.play.update(raw_mux_read);
	}
};
} // namespace Catalyst2::Bootloader
