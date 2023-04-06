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

BOOST_AUTO_TEST_CASE(apint_number)
{
   BOOST_REQUIRE_EQUAL(64, APInt::number::backend_type::limb_bits);
   BOOST_REQUIRE_EQUAL(64, APInt::number::backend_type::internal_limb_count);

   APInt::number zero(0);
   APInt::number negOne(-1);

   BOOST_REQUIRE_EQUAL(0, zero.backend().limbs()[0]);
   BOOST_REQUIRE_EQUAL(1, negOne.backend().limbs()[0]);
   for(auto i = 1U; i < APInt::number::backend_type::internal_limb_count; ++i)
   {
      BOOST_REQUIRE_EQUAL(0, negOne.backend().limbs()[i]);
   }
}

BOOST_AUTO_TEST_CASE(apint_cast)
{
   BOOST_REQUIRE_EQUAL(static_cast<int16_t>(std::numeric_limits<uint16_t>::max()),
                       static_cast<int16_t>(APInt(std::numeric_limits<uint16_t>::max())));
   BOOST_REQUIRE_EQUAL(0b01100110, static_cast<uint8_t>(APInt(0b1111111001100110)));
   BOOST_REQUIRE_EQUAL(53, static_cast<uint8_t>(APInt(53)));
   BOOST_REQUIRE_EQUAL(-34, static_cast<int8_t>(APInt(-34)));
   BOOST_REQUIRE_EQUAL(1u, static_cast<unsigned int>(APInt(std::numeric_limits<unsigned int>::max()) + 2));
   BOOST_REQUIRE_EQUAL(1u, static_cast<int>(APInt(std::numeric_limits<unsigned int>::max()) + 2));
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
   BOOST_REQUIRE_EQUAL(0b0011000000000, APInt(-1) & APInt(0b0011000000000));
   BOOST_REQUIRE_EQUAL(1, (APInt(-27) >> 5) & 1);
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

   BOOST_REQUIRE_EQUAL(APInt(43) << 0, 43);
   BOOST_REQUIRE_EQUAL(APInt(43) << 1, 86);
   BOOST_REQUIRE_EQUAL(APInt(-27) << 0, -27);
   BOOST_REQUIRE_EQUAL(APInt(-27) << 1, -54);
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

BOOST_AUTO_TEST_CASE(apint_leadingOnes)
{
   BOOST_REQUIRE_EQUAL(3, APInt(-27).leadingOnes(8));
   BOOST_REQUIRE_EQUAL(0, APInt(8).leadingOnes(8));
}

BOOST_AUTO_TEST_CASE(apint_leadingZeros)
{
   BOOST_REQUIRE_EQUAL(0, APInt(-1).leadingZeros(8));
   BOOST_REQUIRE_EQUAL(8, APInt(0).leadingZeros(8));
   BOOST_REQUIRE_EQUAL(6, APInt(3).leadingZeros(8));
}

BOOST_AUTO_TEST_CASE(apint_trailingZeros)
{
   BOOST_REQUIRE_EQUAL(0, APInt(1).trailingZeros(8));
   BOOST_REQUIRE_EQUAL(2, APInt(4).trailingZeros(8));
   BOOST_REQUIRE_EQUAL(8, APInt(0).trailingZeros(8));
}

BOOST_AUTO_TEST_CASE(apint_trailingOnes)
{
   BOOST_REQUIRE_EQUAL(1, APInt(1).trailingOnes(8));
   BOOST_REQUIRE_EQUAL(2, APInt(3).trailingOnes(8));
   BOOST_REQUIRE_EQUAL(0, APInt(2).trailingOnes(8));
   BOOST_REQUIRE_EQUAL(8, APInt(-1).trailingOnes(8));
}

BOOST_AUTO_TEST_CASE(apint_minBitwidth)
{
   BOOST_REQUIRE_EQUAL(1, APInt(-1).minBitwidth(true));
   BOOST_REQUIRE_EQUAL(1, APInt(1).minBitwidth(false));
   BOOST_REQUIRE_EQUAL(1, APInt(0).minBitwidth(true));
   BOOST_REQUIRE_EQUAL(1, APInt(0).minBitwidth(false));
   BOOST_REQUIRE_EQUAL(2, APInt(1).minBitwidth(true));
   BOOST_REQUIRE_EQUAL(2, APInt(-2).minBitwidth(true));
   BOOST_REQUIRE_EQUAL(64, APInt(INT64_MAX).minBitwidth(true));
   BOOST_REQUIRE_EQUAL(63, APInt(INT64_MAX).minBitwidth(false));
   BOOST_REQUIRE_EQUAL(65, APInt(UINT64_MAX).minBitwidth(true));
   BOOST_REQUIRE_EQUAL(512, ((APInt(1) << 512) - 1).minBitwidth(false));
}
