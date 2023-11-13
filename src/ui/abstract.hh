#pragma once

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
	virtual void PaintLeds(const Model::OutputBuffer &outs) {
	}

protected:
	void SetButtonLedsCount(uint8_t count, bool on) {
		for (auto i = 0u; i < count; i++)
			c.SetButtonLed(i, on);
	}

	void SetEncoderLedsCount(uint8_t count, uint8_t offset, Color col) {
		for (auto i = 0u; i < count; i++)
			c.SetEncoderLed((i + offset) & 7, col);

		for (auto i = 0u; i < Model::NumChans - count; i++)
			c.SetEncoderLed((count + i + offset) & 7, Colors::off);
	}

	void SetEncoderLedsAddition(uint8_t num, Color col) {
		static constexpr auto max_val = [] {
			uint8_t out = 0;
			for (auto i = 1u; i <= Model::NumChans; i++)
				out += i;
			return out;
		}();

		if (num > max_val)
			return;

		uint8_t t = Model::NumChans;

		while (num >= t) {
			num -= t;
			t -= 1;
			c.SetEncoderLed(t, col);
		}
		c.SetEncoderLed(num - 1, col);
	}

	std::optional<uint8_t> YoungestSceneButton() {
		auto age = 0xffffffffu;
		uint8_t youngest = 0xff;

		for (auto [i, b] : countzip(c.button.scene)) {
			if (!b.is_high())
				continue;

			if (b.time_high > age)
				continue;

			age = b.time_high;
			youngest = i;
		}
		if (youngest == 0xff)
			return std::nullopt;

		return youngest;
	}
};

} // namespace Catalyst2::Ui
