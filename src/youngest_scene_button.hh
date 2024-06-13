#pragma once

#include "controls.hh"
#include <cstdint>
#include <optional>

namespace Catalyst2
{

class YoungestSceneButton : public std::optional<uint8_t> {
	uint8_t _pysb : 7;
	uint8_t _event : 1;

public:
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

		if (youngest == 0xff) {
			std::optional<uint8_t>::operator=(std::nullopt);
		} else {
			std::optional<uint8_t>::operator=(youngest);
		}
		if (prev.has_value() != has_value()) {
			_event = 1;
			_pysb = prev.has_value() ? prev.value() : _pysb;
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
