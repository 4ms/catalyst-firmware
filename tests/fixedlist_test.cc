#include "../src/fixedvector.hh"
#include "doctest.h"

// using namespace Catalyst2;

TEST_CASE("Fixed Linked list") {

	constexpr auto vsize = 4;
	Catalyst2::FixedVector<int, vsize> fv;

	// make sure size is 0 on init
	CHECK(fv.size() == 0);

	// try erasing
	fv.erase(0);
	fv.erase(10);
	CHECK(fv.size() == 0);

	constexpr auto tbyte0 = 16;
	constexpr auto tbyte1 = 31;

	fv.insert(1, tbyte0);
	fv.insert(13, tbyte0);
	CHECK(fv.size() == 0);

	fv.insert(0, tbyte0);
	CHECK(fv.size() == 1);
	CHECK(fv[0] == tbyte0);

	fv.erase(1);
	CHECK(fv.size() == 1);
	fv.erase(0);
	CHECK(fv.size() == 0);

	fv.insert(0, tbyte0);
	fv.insert(1, tbyte1);
	CHECK(fv[0] == tbyte0);
	CHECK(fv[1] == tbyte1);
	CHECK(fv.size() == 2);

	fv.erase(1);
	CHECK(fv[0] == tbyte0);
	CHECK(fv.size() == 1);
	fv.insert(0, tbyte1);
	CHECK(fv.size() == 2);
	CHECK(fv[0] == tbyte1);
	CHECK(fv[1] == tbyte0);

	fv.insert(fv.size(), 0);
	fv.insert(fv.size(), 1);

	CHECK(fv.size() == 4);
	CHECK(fv[0] == tbyte1);
	CHECK(fv[1] == tbyte0);
	CHECK(fv[2] == 0);
	CHECK(fv[3] == 1);

	// try to overflow
	fv.insert(fv.size(), -1);
	CHECK(fv.size() == 4);

	fv.erase(3);
	CHECK(fv.size() == 3);
	fv.erase(0);
	CHECK(fv.size() == 2);
	fv.erase(1);
	CHECK(fv.size() == 1);

	CHECK(fv[0] == tbyte0);
}