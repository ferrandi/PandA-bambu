#include "APInt.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( limits )
{
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint8_t>::max(), APInt::getMaxValue(8));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint8_t>::min(), APInt::getMinValue(8));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint16_t>::max(), APInt::getMaxValue(16));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint16_t>::min(), APInt::getMinValue(16));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint32_t>::max(), APInt::getMaxValue(32));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint32_t>::min(), APInt::getMinValue(32));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint64_t>::max(), APInt::getMaxValue(64));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<uint64_t>::min(), APInt::getMinValue(64));

    BOOST_REQUIRE_EQUAL(std::numeric_limits<int8_t>::max(), APInt::getSignedMaxValue(8));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int8_t>::min(), APInt::getSignedMinValue(8));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int16_t>::max(), APInt::getSignedMaxValue(16));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int16_t>::min(), APInt::getSignedMinValue(16));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int32_t>::max(), APInt::getSignedMaxValue(32));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int32_t>::min(), APInt::getSignedMinValue(32));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int64_t>::max(), APInt::getSignedMaxValue(64));
    BOOST_REQUIRE_EQUAL(std::numeric_limits<int64_t>::min(), APInt::getSignedMinValue(64));
}