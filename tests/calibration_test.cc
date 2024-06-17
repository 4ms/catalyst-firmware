#include "../src/ui/dac_calibration.hh"
#include "../src/macro_value.hh"

#include "doctest.h"
#include <cstdio>

using namespace Catalyst2;

TEST_CASE("Calibration") {
	Calibration::Dac::Data data;
	data.channel[0].slope = 20;
	Model::Output::type test_val = Channel::Output::max;
	CHECK(Calibration::Dac::Process(data.channel[0], test_val) == Channel::Output::max);
}
