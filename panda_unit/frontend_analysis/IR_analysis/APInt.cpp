#include "APInt.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(apint_limits)
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

BOOST_AUTO_TEST_CASE(apint_cast)
{
   BOOST_REQUIRE_EQUAL(static_cast<int16_t>(std::numeric_limits<uint16_t>::max()), APInt(std::numeric_limits<uint16_t>::max()).cast_to<int16_t>());
   BOOST_REQUIRE_EQUAL(0b01100110, APInt(0b1111111001100110).cast_to<uint8_t>());
   BOOST_REQUIRE_EQUAL(53, APInt(53).cast_to<uint8_t>());
   BOOST_REQUIRE_EQUAL(-34, APInt(-34).cast_to<int8_t>());
}

BOOST_AUTO_TEST_CASE(apint_not)
{
   const APInt a = static_cast<int8_t>(0b0011001100);
   const APInt b = static_cast<int8_t>(0b1100110011);
   const APInt c = -1;
   const APInt d = 0;

   BOOST_REQUIRE_EQUAL(a, ~(~a));
   BOOST_REQUIRE_EQUAL(b, ~a);
   BOOST_REQUIRE_EQUAL(a, ~b);
   BOOST_REQUIRE_EQUAL(c, ~d);
   BOOST_REQUIRE_EQUAL(d, ~c);
}

BOOST_AUTO_TEST_CASE(apint_and)
{
   const APInt a = static_cast<int8_t>(0b0011001100);
   const APInt b = static_cast<int8_t>(0b1100110011);
   const APInt c = -1;
   const APInt d = 0;

   BOOST_REQUIRE_EQUAL(a, a & a);
   BOOST_REQUIRE_EQUAL(d, a & b);
   BOOST_REQUIRE_EQUAL(a, a & c);
   BOOST_REQUIRE_EQUAL(d, b & d);
}

BOOST_AUTO_TEST_CASE(apint_ior)
{
   const APInt a = static_cast<int16_t>(0b0011001100110011);
   const APInt b = static_cast<int16_t>(0b1100110011001100);
   const APInt c = -1;
   const APInt d = 0;

   BOOST_REQUIRE_EQUAL(a, a | a);
   BOOST_REQUIRE_EQUAL(c, a | b);
   BOOST_REQUIRE_EQUAL(b, b | d);
   BOOST_REQUIRE_EQUAL(c, a | c);
}

BOOST_AUTO_TEST_CASE(apint_xor)
{
   const APInt a = static_cast<int16_t>(0b0011001100110011);
   const APInt b = static_cast<int16_t>(0b1100110011001100);
   const APInt c = -1;
   const APInt d = 0;

   BOOST_REQUIRE_EQUAL(d, a ^ a);
   BOOST_REQUIRE_EQUAL(c, a ^ b);
   BOOST_REQUIRE_EQUAL(~a, a ^ c);
   BOOST_REQUIRE_EQUAL(b, b ^ d);
}

BOOST_AUTO_TEST_CASE(apint_shl)
{
   const APInt a = 0b00110011001100110011;
   const APInt b = 0b11001100110011001100;
   const APInt c = static_cast<unsigned long long>(-1);
   const APInt d = 0;
   const unsigned long long ca = 0b00110011001100110011;
   const unsigned long long cb = 0b11001100110011001100;
   const unsigned long long cc = static_cast<unsigned long long>(-1);
   const unsigned long long cd = 0;

   BOOST_REQUIRE_EQUAL(b, a << 2);
   BOOST_REQUIRE_EQUAL(ca << 23, a << 23);
   BOOST_REQUIRE_LT(cb << 52, b << 52);
   const APInt sb = b << 52;
   const unsigned long long scb = cb << 52;
   unsigned long long mask = 1;
   for(APInt::bw_t i = 0; i < std::numeric_limits<decltype(cb)>::digits; ++i, mask <<= 1)
   {
      BOOST_REQUIRE_EQUAL(sb.bit_tst(i), static_cast<bool>(scb & mask));
   }
   BOOST_REQUIRE_LT(cc << 36, c << 36);
   const APInt sc = c << 36;
   const unsigned long long scc = cc << 36;
   mask = 1;
   for(APInt::bw_t i = 0; i < std::numeric_limits<decltype(cc)>::digits; ++i, mask <<= 1)
   {
      BOOST_REQUIRE_EQUAL(sc.bit_tst(i), static_cast<bool>(scc & mask));
   }
   BOOST_REQUIRE_EQUAL(cd << 27, d << 27);
   BOOST_REQUIRE_EQUAL(cd << 63, d << 63);
}

BOOST_AUTO_TEST_CASE(apint_shr)
{
   const APInt a = 0b0011001100110011;
   const APInt b = 0b1100110011001100;
   const APInt c = -1;
   const APInt d = 0;
   const long long int ca = 0b0011001100110011;
   const long long int cb = 0b1100110011001100;
   const long long int cc = -1;
   const long long int cd = 0;

   BOOST_REQUIRE_EQUAL(a, b >> 2);
   BOOST_REQUIRE_EQUAL(ca >> 7, a >> 7);
   BOOST_REQUIRE_EQUAL(cb >> 13, b >> 13);
   BOOST_REQUIRE_EQUAL(cc >> 60, c >> 60);
   BOOST_REQUIRE_EQUAL(cd >> 11, d >> 11);
   BOOST_REQUIRE_EQUAL(cd >> 63, d >> 63);
}