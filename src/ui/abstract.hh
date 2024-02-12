#pragma once

#include "clock.hh"
#include "conf/build_options.hh"
#include "conf/model.hh"
#include "conf/palette.hh"
#include "conf/version.hh"
#include "controls.hh"
#include "shared.hh"

namespace Catalyst2::Ui
{
inline void ConfirmCopy(Shared::Interface &p, uint8_t led) {
	p.blinker.Set(led, 8, 250, p.internalclock.TimeNow());
}
inline void ConfirmPaste(Shared::Interface &p, uint8_t led) {
	ConfirmCopy(p, led);
}
inline std::optional<uint8_t> YoungestSceneButton(Controls &c) {
	auto age = 0xffffffffu;
	uint8_t youngest = 0xffu;

	for (auto [i, b] : countzip(c.button.scene)) {
		if (!b.is_high()) {
			continue;
		}
		if (b.time_high > age) {
			continue;
		}
		age = b.time_high;
		youngest = i;
	}
	if (youngest == 0xff) {
		return std::nullopt;
	}
	return youngest;
}
inline void ForEachEncoderInc(Controls &c, auto func) {
	for (auto [i, enc] : countzip(c.encoders)) {
		const auto inc = enc.read();
		if (inc) {
			func(i, inc);
		}
	}
}
inline void ForEachSceneButtonReleased(Controls &c, auto func) {
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
inline void LedBlinker(Controls &c, const Shared::Blinker &b) {
	ClearButtonLeds(c);
	for (auto i = 0u; i < Model::NumChans; i++) {
		c.SetButtonLed(i, b.IsHigh(i));
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

void DisplayRange(Controls &c, Channel::Cv::Range range) {
	const auto half = Model::NumChans / 2u;
	const auto pos = range.PosAmount();
	const auto neg = range.NegAmount();
	const auto posleds = static_cast<uint8_t>(pos * half);
	const auto negleds = static_cast<uint8_t>(neg * half);
	const auto lastposledfade = pos * half - posleds;
	const auto lastnegledfade = neg * half - negleds;
	for (auto i = 0u; i < posleds; i++) {
		c.SetEncoderLed(i + half, Palette::Voltage::Positive);
	}
	c.SetEncoderLed(half + posleds, Palette::off.blend(Palette::Voltage::Positive, lastposledfade));
	for (auto i = 0u; i < negleds; i++) {
		c.SetEncoderLed(half - 1 - i, Palette::Voltage::Negative);
	}
	c.SetEncoderLed(half - negleds - 1, Palette::off.blend(Palette::Voltage::Negative, lastnegledfade));
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

		// 9 = Buttons 1 + 8
		if (version == 9) {
			c.SetButtonLed(7, true);
			c.SetButtonLed(0, true);
		}

		c.SetButtonLed(version - 1, true);
	};

	DisplayVersion(FirmwareMajorVersion);

	while (remaining--) {
		if (remaining == duration / 2)
			DisplayVersion(FirmwareMinorVersion);

		ClearEncoderLeds(c);
		auto phase = (duration - remaining + 0.f) / duration;
		phase *= 8;
		const auto idx = static_cast<uint8_t>(phase);
		const auto cphase = phase - idx;
		const auto col = Palette::Scales::color[idx & 0x7];
		const auto nextcol = idx + 1 >= 8 ? Palette::off : Palette::Scales::color[idx + 1];
		SetEncoderLedsCount(c, 8, 0, col.blend(nextcol, cphase));
		c.Delay(1);
	}
}

class Abstract {
public:
	Controls &c;
	Abstract(Controls &c)
		: c{c} {
	}

	virtual void Init() {
	}
	virtual void Update(Abstract *&interface) = 0;
	virtual void Common() = 0;
	virtual void PaintLeds(const Model::Output::Buffer &outs) {
	}
};

} // namespace Catalyst2::Ui
