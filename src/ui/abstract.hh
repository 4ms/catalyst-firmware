#pragma once

#include "conf/palette.hh"
#include "controls.hh"

namespace Catalyst2::Ui
{

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

protected:
	void ForEachEncoderInc(auto func) {
		for (auto [i, enc] : countzip(c.encoders)) {
			const auto inc = enc.read();
			if (inc) {
				func(i, inc);
			}
		}
	}
	void ForEachSceneButtonReleased(auto func) {
		for (auto [i, b] : countzip(c.button.scene)) {
			if (b.just_went_low()) {
				func(i);
			}
		}
	}
	void ForEachSceneButtonPressed(auto func) {
		for (auto [i, b] : countzip(c.button.scene)) {
			if (b.just_went_high()) {
				func(i);
			}
		}
	}
	void ClearButtonLeds() {
		for (auto i = 0u; i < Model::NumChans; i++) {
			c.SetButtonLed(i, false);
		}
	}

	void ClearEncoderLeds() {
		for (auto i = 0u; i < Model::NumChans; i++) {
			c.SetEncoderLed(i, Palette::off);
		}
	}

	void SetButtonLedsCount(uint8_t count, bool on) {
		for (auto i = 0u; i < count; i++) {
			c.SetButtonLed(i, on);
		}
	}

	void SetEncoderLedsCount(uint8_t count, uint8_t offset, Color col) {
		for (auto i = 0u; i < Model::NumChans; i++) {
			auto color = (i >= offset && i < (offset + count)) ? col : Colors::off;
			c.SetEncoderLed(i, color);
		}
	}

	void SetEncoderLedsFloat(float value, Color col, Color bg_col = Colors::off) {
		value = std::clamp(value, 0.f, 8.f);
		auto fade_led = (unsigned)value;
		float fade_amt = value - fade_led;

		for (auto i = 0u; i < Model::NumChans; i++) {
			auto color = (i == fade_led) ? bg_col.blend(col, fade_amt) : i < fade_led ? col : bg_col;
			c.SetEncoderLed(i, color);
		}
	}

	void SetEncoderLedsAddition(uint8_t num, Color col) {
		static constexpr auto max_val = [] {
			auto out = 0u;
			for (auto i = 1u; i <= Model::NumChans; i++) {
				out += i;
			}
			return out;
		}();

		if (num > max_val) {
			return;
		}
		auto t = Model::NumChans;

		while (num >= t) {
			num -= t;
			t -= 1;
			c.SetEncoderLed(t, col);
		}
		c.SetEncoderLed(num - 1, col);
	}

	std::optional<uint8_t> YoungestSceneButton() {
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

	void DisplayRange(Catalyst2::Channel::Range range) const {
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
};

} // namespace Catalyst2::Ui
