#pragma once

#include "util/debouncer.hh"

namespace Catalyst2
{
enum class SwitchState {
	WasPressed,
	IsPressed,
	WasReleased,
	IsReleased,
};

SwitchState GetSwitchState(Toggler &t)
{
	using enum SwitchState;

	if (t.is_high()) {
		if (t.just_went_high())
			return WasPressed;

		return IsPressed;
	}

	if (t.just_went_low())
		return WasReleased;

	return IsReleased;
}
} // namespace Catalyst2