#include "../src/fixed_list.hh"
#include "doctest.h"

using namespace Catalyst2;

TEST_CASE("Fixed Linked list")
{
	FixedFwList<uint8_t, 5> test;
	test.insert(0, 0);
	test.insert(0, 1);

	CHECK(test.read(0) == 0);
	CHECK(test.read(1) == 1);

	// insert after head
	CHECK(test.insert(0, 1));

	// replace the third element to keep this count going
	test.replace(2, 2);

	// insert antoher after
	CHECK(test.insert(2, 3));

	// insert after previous insert
	CHECK(test.insert(4));

	// try to overfill
	CHECK(test.insert(1, 2) == false);

	// check all the values are correct.
	for (unsigned i = 0; i < test.size(); i++)
		CHECK(test.read(i) == i);

	// try erasing head and tail
	CHECK(test.erase(4));
	CHECK(test.erase(0));
	CHECK(test.erase(0));
	CHECK(test.erase(0));
	CHECK(test.erase(0));

	// cant erase too many
	CHECK(test.erase(0) == false);

	test.insert(0, 2);
	test.insert(3);

	CHECK(test.read(0) == 2);
	CHECK(test.read(1) == 3);
	test.replace(0, 0);
	test.replace(1, 4);
	test.insert(0, 1);
	test.insert(2);
	CHECK(test.insert(3));

	// dont overfill!
	CHECK(test.insert(4) == false);

	// check all the values are correct.
	for (unsigned i = 0; i < test.size(); i++)
		CHECK(test.read(i) == i);

	// erase non head and tail
	CHECK(test.erase(1));
	CHECK(test.erase(1));
	CHECK(test.erase(1));

	// check wrapping index
	// for (unsigned i = 0; i < 10; i += 2) {
	// 	CHECK(test.read(i) == 0);
	// 	CHECK(test.read(i + 1) == 4);
	// }
}