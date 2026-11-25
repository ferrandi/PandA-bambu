#include "utility/bounded_decimal.hpp"

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>

BOOST_AUTO_TEST_CASE(bounded_ascii_decimal)
{
   using panda::utility::bounded_decimal_result;
   using panda::utility::parse_bounded_ascii_decimal;

   std::uint32_t value = 77U;
   BOOST_REQUIRE(parse_bounded_ascii_decimal("1", 1U, 16U, value) == bounded_decimal_result::valid);
   BOOST_REQUIRE_EQUAL(value, 1U);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("16", 1U, 16U, value) == bounded_decimal_result::valid);
   BOOST_REQUIRE_EQUAL(value, 16U);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("00016", 1U, 16U, value) == bounded_decimal_result::valid);
   BOOST_REQUIRE_EQUAL(value, 16U);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("0", 0U, UINT32_MAX, value) == bounded_decimal_result::valid);
   BOOST_REQUIRE_EQUAL(value, 0U);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("4294967295", 0U, UINT32_MAX, value) ==
                 bounded_decimal_result::valid);
   BOOST_REQUIRE_EQUAL(value, UINT32_MAX);

   BOOST_REQUIRE(parse_bounded_ascii_decimal("", 0U, UINT32_MAX, value) == bounded_decimal_result::invalid_format);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("+1", 0U, UINT32_MAX, value) == bounded_decimal_result::invalid_format);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("-1", 0U, UINT32_MAX, value) == bounded_decimal_result::invalid_format);
   BOOST_REQUIRE(parse_bounded_ascii_decimal(" 1", 0U, UINT32_MAX, value) == bounded_decimal_result::invalid_format);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("1 ", 0U, UINT32_MAX, value) == bounded_decimal_result::invalid_format);
   BOOST_REQUIRE(parse_bounded_ascii_decimal(std::string("\xc2\xb2", 2), 0U, UINT32_MAX, value) ==
                 bounded_decimal_result::invalid_format);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("17", 1U, 16U, value) == bounded_decimal_result::out_of_range);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("0", 1U, 16U, value) == bounded_decimal_result::out_of_range);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("4294967296", 0U, UINT32_MAX, value) ==
                 bounded_decimal_result::out_of_range);
   BOOST_REQUIRE(parse_bounded_ascii_decimal(std::string(1000, '9'), 0U, UINT32_MAX, value) ==
                 bounded_decimal_result::out_of_range);
   BOOST_REQUIRE(parse_bounded_ascii_decimal("999999999999999999999x", 0U, UINT32_MAX, value) ==
                 bounded_decimal_result::invalid_format);
}
