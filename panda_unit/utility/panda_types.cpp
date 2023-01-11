#include "panda_types.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(integer_cst_bitsize)
{
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(1), 1);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(0), 1);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(-1), 1);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(7), 3);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(9), 4);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(-8), 4);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(-7), 4);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(-32), 6);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(-141733920769), 39);
   BOOST_REQUIRE_EQUAL(panda::integer_cst_bitsize(INT64_MIN), 64);
}

BOOST_AUTO_TEST_CASE(integer_cst_cast)
{
   BOOST_REQUIRE_EQUAL(1u, static_cast<unsigned int>(integer_cst_t(std::numeric_limits<unsigned int>::max()) + 2));
}

BOOST_AUTO_TEST_CASE(integer_cst_mask)
{
   BOOST_REQUIRE_EQUAL(7, integer_cst_t(-1) & ((integer_cst_t(1) << 3) - 1));
   BOOST_REQUIRE_EQUAL(std::numeric_limits<unsigned int>::max(),
                       static_cast<unsigned int>(integer_cst_t(std::numeric_limits<unsigned int>::max()) & -1));
}