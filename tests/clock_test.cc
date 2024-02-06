#include "../src/clock.hh"

#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("clock phase") {
	Clock::Bpm::type bpm;
	bpm.Set(Clock::BpmToTicks(64000));
	Clock::Bpm clock{bpm};

	Clock::Divider::type cdiv;
	cdiv.Inc(3);

	Clock::Divider divider;

	uint32_t step = 1;

	fprintf(stderr, "C Div Phase: %f\n", divider.GetPhase(cdiv));

	for (;;) {
		clock.Update();

		if (clock.Output()) {

			divider.Update(cdiv);
			fprintf(stderr, "C Div Phase: %f\n", divider.GetPhase(cdiv));

			if (divider.Step()) {
				step += 1;
			}

			if (step == 2)
				break;
		}
		fprintf(
			stderr, "Clock Phase: %f Actual Phase: %f\n", clock.GetPhase(), divider.GetPhase(cdiv, clock.GetPhase()));
	}
}
