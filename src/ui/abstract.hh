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
	virtual void OnSceneButtonRelease(uint8_t button) {
	}
	virtual void OnEncoderInc(uint8_t encoder, int32_t inc) {
	}
	virtual void PaintLeds(const Model::Output::Buffer &outs) {
	}

protected:
	void ConfirmCopy(uint8_t led) {
		static constexpr auto num_flashes = 8u;
		static constexpr auto time_ms = 500u;
		for (auto i = 0u; i < num_flashes; i++) {
			c.SetButtonLed(led, static_cast<bool>(i & 0x01));
			c.Delay(time_ms / num_flashes);
		}
	}
	void ConfirmPaste(uint8_t led) {
		ConfirmCopy(led);
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
		for (auto i = 0u; i < count; i++) {
			c.SetEncoderLed((i + offset) & 7, col);
		}
		for (auto i = 0u; i < Model::NumChans - count; i++) {
			c.SetEncoderLed((count + i + offset) & 7, Colors::off);
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
