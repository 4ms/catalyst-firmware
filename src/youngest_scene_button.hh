#pragma once

#include "controls.hh"
#include <cstdint>
#include <optional>

namespace Catalyst2
{

class YoungestSceneButton : public std::optional<uint8_t> {
	uint8_t _pysb : 7 = 0;
	uint8_t _event : 1;

public:
	YoungestSceneButton()
		: std::optional<uint8_t>{0} {
	}
	void Update(Controls &c) {
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

		const auto prev = std::optional<uint8_t>::operator=(*this);
		auto &cur = std::optional<uint8_t>::operator=(*this);

		if (youngest == 0xff) {
			cur = std::nullopt;
		} else {
			cur = youngest;
		}
		_pysb = has_value() ? value() : _pysb;

		if (prev != cur) {
			_event = 1;
		}
	}
	bool Event() {
		const auto out = _event;
		_event = 0;
		return out;
	}
	uint8_t Last() const {
		return _pysb;
	}
};

} // namespace Catalyst2
