#pragma once

#include "clock.hh"
#include "conf/build_options.hh"
#include "conf/palette.hh"
#include "conf/version.hh"
#include "controls.hh"

namespace Catalyst2::Ui
{
inline void ClearEncoderEvents(Controls &c) {
	for (auto &e : c.encoders) {
		static_cast<void>(e.read());
	}
}
inline void ForEachEncoderInc(Controls &c, auto func) {
	for (auto [i, enc] : countzip(c.encoders)) {
		const auto inc = enc.read();
		if (inc) {
			func(i, inc);
		}
	}
}
inline void ForEachSceneButtonJustReleased(Controls &c, auto func) {
	for (auto [i, b] : countzip(c.button.scene)) {
		if (b.just_went_low()) {
			func(i);
		}
	}
}
inline void ForEachSceneButtonJustPressed(Controls &c, auto func) {
	for (auto [i, b] : countzip(c.button.scene)) {
		if (b.just_went_high()) {
			func(i);
		}
	}
}
inline void ForEachSceneButtonPressed(Controls &c, auto func) {
	for (auto [i, b] : countzip(c.button.scene)) {
		if (b.is_pressed()) {
			func(i);
		}
	}
}
inline void ClearButtonLeds(Controls &c) {
	for (auto i = 0u; i < Model::NumChans; i++) {
		c.SetButtonLed(i, false);
	}
}

inline void ClearEncoderLeds(Controls &c) {
	for (auto i = 0u; i < Model::NumChans; i++) {
		c.SetEncoderLed(i, Palette::off);
	}
}

inline void SetButtonLedsCount(Controls &c, uint8_t count, bool on) {
	for (auto i = 0u; i < count; i++) {
		c.SetButtonLed(i, on);
	}
}

inline void SetEncoderLedsCount(Controls &c, uint8_t count, uint8_t offset, Color col) {
	for (auto i = 0u; i < Model::NumChans; i++) {
		auto color = (i >= offset && i < (offset + count)) ? col : Colors::off;
		c.SetEncoderLed(i, color);
	}
}

inline void SetEncoderLedsFloat(Controls &c, float value, Color col, Color bg_col = Colors::off) {
	value = std::clamp(value, 0.f, 8.f);
	auto fade_led = (unsigned)value;
	float fade_amt = value - fade_led;

	for (auto i = 0u; i < Model::NumChans; i++) {
		auto color = (i == fade_led) ? bg_col.blend(col, fade_amt) : i < fade_led ? col : bg_col;
		c.SetEncoderLed(i, color);
	}
}

inline void DisplayRange(Controls &c, Channel::Cv::Range range) {
	const auto half = Model::NumChans / 2u;
	const auto pos = range.PosAmount();
	const auto neg = range.NegAmount();
	const auto posleds = static_cast<uint8_t>(pos * half);
	const auto negleds = static_cast<uint8_t>(neg * half);
	auto lastposledfade = pos * half - posleds;
	auto lastnegledfade = neg * half - negleds;
	lastposledfade *= lastposledfade;
	lastnegledfade *= lastnegledfade;
	const auto PositiveColor = Palette::Range::color(range);
	const auto NegativeColor = Palette::Range::Negative;
	for (auto i = 0u; i < posleds; i++) {
		c.SetEncoderLed(i + half, PositiveColor);
	}
	c.SetEncoderLed(half + posleds, Palette::off.blend(PositiveColor, lastposledfade));
	for (auto i = 0u; i < negleds; i++) {
		c.SetEncoderLed(half - 1 - i, NegativeColor);
	}
	c.SetEncoderLed(half - negleds - 1, Palette::off.blend(NegativeColor, lastnegledfade));
}

inline void SetLedsClockDiv(Controls &c, uint32_t div) {
	div -= 1;
	auto idx = 0u;
	while (div >= 64) {
		div -= 64;
		idx += 1;
	}

	auto page = 0;
	c.SetButtonLed(page, true);
	while (div >= Model::NumChans) {
		div -= Model::NumChans;
		page += 1;
		c.SetButtonLed(page, true);
	}
	const auto col = Palette::Setting::ClockDiv::color[idx];
	c.SetEncoderLed(div, col);
}

inline void StartupAnimation(Controls &c) {
	if constexpr (BuildOptions::skip_startup_animation) {
		return;
	}
	const auto duration = 1000;
	auto remaining = duration;

	auto DisplayVersion = [&c](unsigned version) {
		ClearButtonLeds(c);

		// Display units digit only
		if (version >= 10)
			version = version % 10;

		// 0 = All buttons off
		if (version == 0)
			return;

		else if (version == 9) {
			// 9 = Buttons 1 + 8
			c.SetButtonLed(7, true);
			c.SetButtonLed(0, true);
		} else
			c.SetButtonLed(version - 1, true);
	};

	DisplayVersion(FirmwareMajorVersion);

	while (remaining--) {
		if (remaining == duration / 2)
			DisplayVersion(FirmwareMinorVersion);

		for (auto enc = 0u; enc < 8; enc++) {
			auto phase = (duration - remaining + enc * (duration / 32.f)) / duration;
			phase = std::clamp(phase * 8, 0.f, 7.999f);
			auto idx = static_cast<uint8_t>(phase);
			const auto cphase = phase - idx;
			const auto col = Palette::Scales::color[idx + 1];
			const auto nextcol = Palette::Scales::color[idx + 2];
			c.SetEncoderLed(enc, col.blend(nextcol, cphase));
		}
		c.Delay(1);
		c.WriteButtonLeds();
	}
}
} // namespace Catalyst2::Ui
