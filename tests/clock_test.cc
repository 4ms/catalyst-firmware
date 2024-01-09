#include "../src/clock.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("clock phase") {
	Clock::Bpm::type bpm;
	Clock::Bpm clock{bpm};

	Clock::Divider::type cdiv;
	cdiv.Inc(16);

	Clock::Divider divider;

	uint32_t step = 1;

	for (;;) {
		clock.Update();

		if (clock.Output()) {

			fprintf(stderr, "Clock Divider Phase: %f\n", divider.GetPhase(cdiv));
			fprintf(stderr, "Actual Phase: %f\n", divider.GetPhase(cdiv) * step);

			divider.Update(cdiv);

			if (divider.Step()) {
				step += 1;
			}

			if (step == 2)
				break;
		}
	}
}
