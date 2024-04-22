#include "../src/clock.hh"

#include "doctest.h"
#include <cstdio>

using namespace Catalyst2;

TEST_CASE("clock phase") {
	Clock::Bpm::Data bpm;
	Clock::Bpm::Interface clock{bpm};

	Clock::Divider::type cdiv;
	cdiv.Inc(3);

	Clock::Divider divider;

	uint32_t step = 1;

	// fprintf(stderr, "C Div Phase: %f\n", divider.GetPhase(cdiv));

	for (;;) {

		if (clock.Update()) {

			// fprintf(stderr, "C Div Phase: %f\n", divider.GetPhase(cdiv));

			if (divider.Update(cdiv)) {
				step += 1;
			}

			if (step == 2)
				break;
		}
		// fprintf(stderr, "Clock Phase: %f Actual Phase: %f\n", clock.GetPhase(), divider.GetPhase(cdiv,
		// clock.GetPhase()));
	}
}
