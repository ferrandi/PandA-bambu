#include "Range.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(range)
{
   RangeRef r, a;
   // Full range
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(0, r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(255, r->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(-128, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(127, r->getSignedMax());

   // Regular pos-pos
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, 10, 113)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(10, r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(113, r->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(10, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(113, r->getSignedMax());

   // Regular pos-pos unsigned
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, 129, 236)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(129, r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(236, r->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(static_cast<int8_t>(129), r->getSignedMin());
   BOOST_REQUIRE_EQUAL(static_cast<int8_t>(236), r->getSignedMax());

   // Regular neg-pos
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -5, 113)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(0, r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(255, r->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(-5, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(113, r->getSignedMax());

   // Regular neg-pos overflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -5, 132)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isAnti());
   BOOST_REQUIRE_EQUAL(0, r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(255, r->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(-128, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(127, r->getSignedMax());
   a = r->getAnti();
   BOOST_REQUIRE_EQUAL(-123, a->getSignedMin());
   BOOST_REQUIRE_EQUAL(-6, a->getSignedMax());

   // Regular neg-pos full overflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -3, 252)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(r->isFullSet());

   // Regular neg-neg
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -111, -36)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(-111), r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(-36), r->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(-111, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(-36, r->getSignedMax());

   // Regular neg-neg underflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -130, -36)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isAnti());
   a = r->getAnti();
   BOOST_REQUIRE_EQUAL(-35, a->getSignedMin());
   BOOST_REQUIRE_EQUAL(125, a->getSignedMax());

   // Regular neg-neg full underflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -292, -36)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(r->isFullSet());
   BOOST_REQUIRE(r->isRegular());

   // Anti pos-pos
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, 10, 113)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isAnti());
   a = r->getAnti();
   BOOST_REQUIRE_EQUAL(10, a->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(113, a->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(10, a->getSignedMin());
   BOOST_REQUIRE_EQUAL(113, a->getSignedMax());

   // Anti pos-pos unsigned
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, 132, 213)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isAnti());
   a = r->getAnti();
   BOOST_REQUIRE_EQUAL(132, a->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(213, a->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(static_cast<int8_t>(132), a->getSignedMin());
   BOOST_REQUIRE_EQUAL(static_cast<int8_t>(213), a->getSignedMax());

   // Anti neg-pos
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -34, 97)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isAnti());
   BOOST_REQUIRE_EQUAL(98, r->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(-35), r->getUnsignedMax());
   a = r->getAnti();
   BOOST_REQUIRE_EQUAL(-34, a->getSignedMin());
   BOOST_REQUIRE_EQUAL(97, a->getSignedMax());

   // Anti neg-pos overflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -34, 210)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(-45, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(-35, r->getSignedMax());

   // Anti neg-pos full overflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -34, 230)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isAnti());
   BOOST_REQUIRE(r->isFullSet());

   // Anti neg-neg
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -104, -67)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isAnti());
   a = r->getAnti();
   BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(-104), a->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(-67), a->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(-104, a->getSignedMin());
   BOOST_REQUIRE_EQUAL(-67, a->getSignedMax());

   // Anti neg-neg underflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -130, -67)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(-66, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(125, r->getSignedMax());

   // Anti neg-neg full underflow
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -323, -67)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(r->isFullSet());
   BOOST_REQUIRE(r->isRegular());

   // Anti min-neg
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, Range::Min, -57)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(-56, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(127, r->getSignedMax());
   BOOST_REQUIRE_EQUAL(127, r->getUpper());

   // Anti min-pos
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, Range::Min, 13)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(14, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(127, r->getSignedMax());
   BOOST_REQUIRE_EQUAL(127, r->getUpper());

   // Anti pos-max
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, 32, Range::Max)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(31, r->getSignedMax());
   BOOST_REQUIRE_EQUAL(-128, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(-128, r->getLower());

   // Anti neg-max
   BOOST_REQUIRE_NO_THROW(r.reset(new Range(Anti, 8, -33, Range::Max)));
   BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
   BOOST_REQUIRE(!r->isFullSet());
   BOOST_REQUIRE(r->isRegular());
   BOOST_REQUIRE_EQUAL(-34, r->getSignedMax());
   BOOST_REQUIRE_EQUAL(-128, r->getSignedMin());
   BOOST_REQUIRE_EQUAL(-128, r->getLower());

   const auto antiZero = Range(Anti, 1, 0, 0);
   const auto antiOne = Range(Anti, 1, 1, 1);
   BOOST_REQUIRE_EQUAL(1, antiZero.getUnsignedMin());
   BOOST_REQUIRE_EQUAL(0, antiOne.getUnsignedMin());
}

BOOST_AUTO_TEST_CASE(range_span)
{
   RangeRef a(new Range(Regular, 8, 5, 12));
   RangeRef b(new Range(Regular, 8, -50, -20));
   RangeRef c(new Range(Regular, 8, -23, 12));
   RangeRef d(new Range(Anti, 8, -5, 7));
   RangeRef e(new Range(Regular, 8));
   RangeRef f(new Range(Unknown, 8));
   RangeRef g(new Range(Anti, 8, -5, 6));
   RangeRef h(new Range(Anti, 32, -55726, 0));
   RangeRef i(new Range(Anti, 32, -55727, 0));
   RangeRef j(new Range(Anti, 8, -1, -1));
   RangeRef k(new Range(Anti, 8, Range::Min, -1));

   BOOST_REQUIRE_EQUAL(8, a->getSpan());
   BOOST_REQUIRE_EQUAL(31, b->getSpan());
   BOOST_REQUIRE_EQUAL(36, c->getSpan());
   BOOST_REQUIRE_EQUAL(243, d->getSpan());
   BOOST_REQUIRE_EQUAL(256, e->getSpan());
   BOOST_REQUIRE_EQUAL(256, f->getSpan());
   BOOST_REQUIRE_EQUAL(244, g->getSpan());
   BOOST_REQUIRE_EQUAL(255, j->getSpan());
   BOOST_REQUIRE_EQUAL(128, k->getSpan());
   BOOST_REQUIRE_GT(g->getSpan(), d->getSpan());
   BOOST_REQUIRE_GT(h->getSpan(), i->getSpan());
}

BOOST_AUTO_TEST_CASE(range_neededBits)
{
   BOOST_REQUIRE_EQUAL(15, Range::neededBits(INT16_MAX, 8, false));
   BOOST_REQUIRE_EQUAL(16, Range::neededBits(INT16_MIN, 57, true));
   BOOST_REQUIRE_EQUAL(16, Range::neededBits(UINT16_MAX, 8, false));
   BOOST_REQUIRE_EQUAL(64, Range::neededBits(UINT64_MAX, 957, false));
   BOOST_REQUIRE_EQUAL(6, Range::neededBits(-32, -5, true));
   BOOST_REQUIRE_EQUAL(5, Range::neededBits(5, 31, false));
}

BOOST_AUTO_TEST_CASE(range_bitvalues)
{
   const Range test1(Regular, 8, 8, 10);
   const Range test2(Regular, 8, -8, -5);
   const Range test4(Regular, 16, 126, 157);
   const Range test5(Regular, 8);

   BOOST_REQUIRE_EQUAL("10UU", bitstring_to_string(test1.getBitValues(false)));
   BOOST_REQUIRE_EQUAL("111110UU", bitstring_to_string(test2.getBitValues(false)));
   BOOST_REQUIRE_EQUAL("10UU", bitstring_to_string(test2.getBitValues(true)));
   BOOST_REQUIRE_EQUAL("UUUUUUUU", bitstring_to_string(test4.getBitValues(false)));
   BOOST_REQUIRE_EQUAL("0UUUUUUUU", bitstring_to_string(test4.getBitValues(true)));
   BOOST_REQUIRE_EQUAL("UUUUUUUU", bitstring_to_string(test5.getBitValues(false)));
   BOOST_REQUIRE_EQUAL("UUUUUUUU", bitstring_to_string(test5.getBitValues(true)));

   RealRange test3(Range(Regular, 1, 0, 0), Range(Regular, 8, 0b01101110, 0b01111111), Range(Regular, 23, 0b0, 0b11111111111110000000110));

   BOOST_REQUIRE_EQUAL(bitstring_to_string(test3.getBitValues(true)), bitstring_to_string(test3.getBitValues(false)));
   BOOST_REQUIRE_EQUAL("0011UUUUUUUUUUUUUUUUUUUUUUUUUUUU", bitstring_to_string(test3.getBitValues(true)));

   const auto bv1 = Range::fromBitValues(string_to_bitstring("10000000010"), 11, false);
   BOOST_REQUIRE(bv1->isConstant());
   BOOST_REQUIRE_EQUAL(bv1->getBitWidth(), 11);
   BOOST_REQUIRE_EQUAL(bv1->getUnsignedMax(), 0b10000000010);

   const auto bv2 = Range::fromBitValues(string_to_bitstring("10XXX"), 5, true);
   BOOST_REQUIRE_EQUAL(5, bv2->getBitWidth());
   BOOST_REQUIRE_EQUAL(16, bv2->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(23, bv2->getUnsignedMax());

   const auto bv3 = Range::fromBitValues(string_to_bitstring("UUUUXX"), 8, true);
   BOOST_REQUIRE_EQUAL(8, bv3->getBitWidth());
   BOOST_REQUIRE_EQUAL(-32, bv3->getSignedMin());
   BOOST_REQUIRE_EQUAL(31, bv3->getSignedMax());
   BOOST_REQUIRE_EQUAL(0, bv3->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(255, bv3->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE(range_addition)
{
   RangeRef aPositive(new Range(Regular, 8, 5, 9));
   RangeRef aNegative(new Range(Regular, 8, -9, -5));
   RangeRef aMix(new Range(Regular, 8, -5, 9));
   RangeRef bPositive(new Range(Regular, 8, 3, 6));
   RangeRef bNegative(new Range(Regular, 8, -6, -3));
   RangeRef bMix(new Range(Regular, 8, -3, 6));

   auto addPaPb = aPositive->add(bPositive);
   auto addPbPa = bPositive->add(aPositive);
   BOOST_REQUIRE(addPaPb->isSameRange(addPbPa));
   BOOST_REQUIRE_EQUAL(addPaPb->getBitWidth(), aPositive->getBitWidth());
   BOOST_REQUIRE_EQUAL(15, addPaPb->getSignedMax());
   BOOST_REQUIRE_EQUAL(8, addPaPb->getSignedMin());

   auto addNaNb = aNegative->add(bNegative);
   auto addNbNa = bNegative->add(aNegative);
   BOOST_REQUIRE(addNaNb->isSameRange(addNbNa));
   BOOST_REQUIRE_EQUAL(-15, addNaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(-8, addNaNb->getSignedMax());

   auto addMaMb = aMix->add(bMix);
   auto addMbMa = bMix->add(aMix);
   BOOST_REQUIRE(addMaMb->isSameRange(addMbMa));
   BOOST_REQUIRE_EQUAL(15, addMaMb->getSignedMax());
   BOOST_REQUIRE_EQUAL(-8, addMaMb->getSignedMin());

   auto addPaNb = aPositive->add(bNegative);
   auto addNbPa = bNegative->add(aPositive);
   BOOST_REQUIRE(addPaNb->isSameRange(addNbPa));
   BOOST_REQUIRE_EQUAL(-1, addPaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(6, addPaNb->getSignedMax());

   RangeRef limitU63(new Range(Regular, 64, 0ULL, 9223372036854775807ULL));

   auto addLimitU64 = limitU63->add(RangeRef(new Range(Regular, 64, 0, 1)));
   BOOST_REQUIRE_EQUAL(64, addLimitU64->getBitWidth());
   BOOST_REQUIRE_EQUAL(9223372036854775808ULL, addLimitU64->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(0, addLimitU64->getUnsignedMin());
}

BOOST_AUTO_TEST_CASE(range_subtraction)
{
   RangeRef aPositive(new Range(Regular, 8, 5, 9));
   RangeRef aNegative(new Range(Regular, 8, -9, -5));
   RangeRef aMix(new Range(Regular, 8, -5, 9));
   RangeRef bPositive(new Range(Regular, 8, 3, 6));
   RangeRef bNegative(new Range(Regular, 8, -6, -3));
   RangeRef bMix(new Range(Regular, 8, -3, 6));

   auto subPaPb = aPositive->sub(bPositive);
   BOOST_REQUIRE_EQUAL(subPaPb->getBitWidth(), aPositive->getBitWidth());
   BOOST_REQUIRE_EQUAL(6, subPaPb->getSignedMax());
   BOOST_REQUIRE_EQUAL(-1, subPaPb->getSignedMin());

   auto subNaNb = aNegative->sub(bNegative);
   BOOST_REQUIRE_EQUAL(-6, subNaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(1, subNaNb->getSignedMax());

   auto subMaMb = aMix->sub(bMix);
   BOOST_REQUIRE_EQUAL(-11, subMaMb->getSignedMin());
   BOOST_REQUIRE_EQUAL(12, subMaMb->getSignedMax());

   auto subPaNb = aPositive->sub(bNegative);
   BOOST_REQUIRE_EQUAL(8, subPaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(15, subPaNb->getSignedMax());
}

BOOST_AUTO_TEST_CASE(range_multiplication)
{
   RangeRef aPositive(new Range(Regular, 8, 5, 9));
   RangeRef aNegative(new Range(Regular, 8, -9, -5));
   RangeRef aMix(new Range(Regular, 8, -5, 9));
   RangeRef bPositive(new Range(Regular, 8, 3, 6));
   RangeRef bNegative(new Range(Regular, 8, -6, -3));
   RangeRef bMix(new Range(Regular, 8, -3, 6));

   auto mulPaPb = aPositive->mul(bPositive);
   auto mulPbPa = bPositive->mul(aPositive);
   BOOST_REQUIRE(mulPaPb->isSameRange(mulPbPa));
   BOOST_REQUIRE_EQUAL(mulPaPb->getBitWidth(), aPositive->getBitWidth());
   BOOST_REQUIRE_EQUAL(54, mulPaPb->getSignedMax());
   BOOST_REQUIRE_EQUAL(15, mulPaPb->getSignedMin());

   auto mulNaNb = aNegative->mul(bNegative);
   auto mulNbNa = bNegative->mul(aNegative);
   BOOST_REQUIRE(mulNaNb->isSameRange(mulNbNa));
   BOOST_REQUIRE_EQUAL(15, mulNaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(54, mulNaNb->getSignedMax());

   auto mulMaMb = aMix->mul(bMix);
   auto mulMbMa = bMix->mul(aMix);
   BOOST_REQUIRE(mulMaMb->isSameRange(mulMbMa));
   BOOST_REQUIRE_EQUAL(-30, mulMaMb->getSignedMin());
   BOOST_REQUIRE_EQUAL(54, mulMaMb->getSignedMax());

   auto mulPaNb = aPositive->mul(bNegative);
   auto mulNbPa = bNegative->mul(aPositive);
   BOOST_REQUIRE(mulPaNb->isSameRange(mulNbPa));
   BOOST_REQUIRE_EQUAL(-54, mulPaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(-15, mulPaNb->getSignedMax());

   RangeRef big(new Range(Regular, 8, 57, 85));

   auto mulPaBig = aPositive->mul(big);
   BOOST_REQUIRE(mulPaBig->isFullSet());

   auto mulNaBig = aNegative->mul(big);
   BOOST_REQUIRE(mulNaBig->isFullSet());

   auto mulMaBig = aMix->mul(big);
   BOOST_REQUIRE(mulMaBig->isFullSet());
}

BOOST_AUTO_TEST_CASE(range_division)
{
   RangeRef aPositive(new Range(Regular, 8, 5, 9));
   RangeRef aNegative(new Range(Regular, 8, -9, -5));
   RangeRef aMix(new Range(Regular, 8, -5, 9));
   RangeRef bPositive(new Range(Regular, 8, 3, 6));
   RangeRef bNegative(new Range(Regular, 8, -6, -3));
   RangeRef bMix(new Range(Regular, 8, -3, 6));

   RangeRef invariant(new Range(Regular, 8, 1, 1));

   auto sdivPaI = aPositive->sdiv(invariant);
   BOOST_REQUIRE(aPositive->isSameRange(sdivPaI));

   auto sdivPaNb = aPositive->sdiv(bNegative);
   BOOST_REQUIRE_EQUAL(-3, sdivPaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, sdivPaNb->getSignedMax());

   auto udivPaNb = aPositive->udiv(bNegative);
   BOOST_REQUIRE_EQUAL(0, udivPaNb->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(0, udivPaNb->getSignedMin());

   RangeRef big(new Range(Regular, 8, 57, 85));

   auto sdivPaBig = aPositive->sdiv(big);
   BOOST_REQUIRE(sdivPaBig->isConstant());
   BOOST_REQUIRE_EQUAL(0, sdivPaBig->getUnsignedMin());
}

BOOST_AUTO_TEST_CASE(range_reminder)
{
   RangeRef aPositive(new Range(Regular, 8, 5, 9));
   RangeRef aNegative(new Range(Regular, 8, -9, -5));
   RangeRef aMix(new Range(Regular, 8, -5, 9));
   RangeRef bPositive(new Range(Regular, 8, 3, 6));
   RangeRef bNegative(new Range(Regular, 8, -6, -3));
   RangeRef bMix(new Range(Regular, 8, -3, 6));

   RangeRef invariant(new Range(Regular, 8, 1, 1));

   auto sremPaI = aPositive->srem(invariant);
   BOOST_REQUIRE_EQUAL(0, sremPaI->getSignedMax());
   BOOST_REQUIRE_EQUAL(0, sremPaI->getSignedMin());

   auto sremPaPb = aPositive->srem(bPositive);
   BOOST_REQUIRE_EQUAL(0, sremPaPb->getSignedMin());
   BOOST_REQUIRE_EQUAL(5, sremPaPb->getSignedMax());

   auto sremPaNb = aPositive->srem(bNegative);
   BOOST_REQUIRE_EQUAL(0, sremPaNb->getSignedMin());
   BOOST_REQUIRE_EQUAL(5, sremPaNb->getSignedMax());
}

BOOST_AUTO_TEST_CASE(range_shl)
{
   RangeRef pos(new Range(Regular, 8, 12, 57));
   RangeRef mix(new Range(Regular, 8, -27, 43));
   RangeRef neg(new Range(Regular, 8, -105, -39));
   RangeRef allOne(new Range(Regular, 8, -1, -1));
   RangeRef antiPos(new Range(Anti, 8, 5, 38));
   RangeRef antiNeg(new Range(Anti, 8, -47, -15));
   RangeRef antiMix(new Range(Anti, 8, -17, 0));
   RangeRef zero(new Range(Regular, 8, 0, 0));
   RangeRef zeroOne(new Range(Regular, 8, 0, 1));
   RangeRef zeroSeven(new Range(Regular, 8, 0, 7));
   RangeRef eight(new Range(Regular, 8, 8, 8));
   RangeRef two(new Range(Regular, 8, 2, 2));

   auto posShl = pos->shl(zeroOne);
   BOOST_REQUIRE(posShl->isRegular());
   BOOST_REQUIRE_EQUAL(12, posShl->getSignedMin());
   BOOST_REQUIRE_EQUAL(114, posShl->getSignedMax());
   posShl = pos->shl(eight);
   BOOST_REQUIRE(posShl->isConstant());
   BOOST_REQUIRE_EQUAL(0, posShl->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, posShl->getSignedMax());
   posShl = pos->shl(zero);
   BOOST_REQUIRE(posShl->isSameRange(pos));

   auto negShl = neg->shl(zeroSeven);
   BOOST_REQUIRE(negShl->isFullSet());
   negShl = neg->shl(eight);
   BOOST_REQUIRE(negShl->isConstant());
   BOOST_REQUIRE_EQUAL(0, negShl->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, negShl->getSignedMax());
   negShl = neg->shl(zero);
   BOOST_REQUIRE(negShl->isSameRange(neg));

   auto mixShl = mix->shl(zeroOne);
   BOOST_REQUIRE(mixShl->isRegular());
   BOOST_REQUIRE_EQUAL(86, mixShl->getSignedMax());
   BOOST_REQUIRE_EQUAL(-54, mixShl->getSignedMin());
   mixShl = mix->shl(eight);
   BOOST_REQUIRE(mixShl->isConstant());
   BOOST_REQUIRE_EQUAL(0, mixShl->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, mixShl->getSignedMax());
   mixShl = mix->shl(zero);
   BOOST_REQUIRE(mixShl->isSameRange(mix));

   auto allOneShl = allOne->shl(zeroSeven);
   BOOST_REQUIRE(allOneShl->isRegular());
   BOOST_REQUIRE_EQUAL(-1, allOneShl->getSignedMax());
   BOOST_REQUIRE_EQUAL(-128, allOneShl->getSignedMin());
   allOneShl = allOne->shl(eight);
   BOOST_REQUIRE(allOneShl->isConstant());
   BOOST_REQUIRE_EQUAL(0, allOneShl->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, allOneShl->getSignedMax());
}

BOOST_AUTO_TEST_CASE(range_abs)
{
   RangeRef positive(new Range(Regular, 8, 5, 12));
   RangeRef safeMix(new Range(Regular, 8, -12, 5));
   RangeRef unsafeMix(new Range(Regular, 8, -128, 31));
   RangeRef safeNeg(new Range(Regular, 8, -15, -3));
   RangeRef unsafeNeg(new Range(Regular, 8, -128, -57));
   RangeRef antiNeg(new Range(Anti, 8, -57, -35));
   RangeRef antiSafe(new Range(Anti, 8, -128, 0));

   auto posAbs = positive->abs();
   BOOST_REQUIRE(posAbs->isSameRange(positive));

   auto safeMixAbs = safeMix->abs();
   BOOST_REQUIRE(safeMixAbs->isRegular());
   BOOST_REQUIRE_EQUAL(0, safeMixAbs->getSignedMin());
   BOOST_REQUIRE_EQUAL(12, safeMixAbs->getSignedMax());

   auto unsMixAbs = unsafeMix->abs();
   BOOST_REQUIRE(unsMixAbs->isAnti());
   BOOST_REQUIRE_EQUAL(-128, unsMixAbs->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, unsMixAbs->getUnsignedMin());

   auto negAbs = safeNeg->abs();
   BOOST_REQUIRE(negAbs->isRegular());
   BOOST_REQUIRE_EQUAL(3, negAbs->getSignedMin());
   BOOST_REQUIRE_EQUAL(15, negAbs->getSignedMax());

   auto unNegAbs = unsafeNeg->abs();
   BOOST_REQUIRE(unNegAbs->isAnti());
   BOOST_REQUIRE_EQUAL(-128, unNegAbs->getSignedMin());
   BOOST_REQUIRE_EQUAL(57, unNegAbs->getUnsignedMin());

   auto antiNegAbs = antiNeg->abs();
   BOOST_REQUIRE(antiNegAbs->isAnti());
   BOOST_REQUIRE_EQUAL(128, antiNegAbs->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(0, antiNegAbs->getUnsignedMin());

   auto antiSafeAbs = antiSafe->abs();
   BOOST_REQUIRE(antiSafeAbs->isRegular());
   BOOST_REQUIRE_EQUAL(1, antiSafeAbs->getSignedMin());
   BOOST_REQUIRE_EQUAL(127, antiSafeAbs->getSignedMax());
}

BOOST_AUTO_TEST_CASE(range_negate)
{
   RangeRef pos(new Range(Regular, 8, 5, 50));
   RangeRef neg(new Range(Regular, 8, -50, -5));
   RangeRef mix(new Range(Regular, 8, -50, 37));
   RangeRef unsafe(new Range(Regular, 8, -128, 5));
   RangeRef antiPos(new Range(Anti, 8, 6, 10));
   RangeRef antiNeg(new Range(Anti, 8, -50, -26));
   RangeRef antiMix(new Range(Anti, 8, -30, 57));

   auto posNeg = pos->negate();
   BOOST_REQUIRE(posNeg->isRegular());
   BOOST_REQUIRE_EQUAL(-5, posNeg->getSignedMax());
   BOOST_REQUIRE_EQUAL(-50, posNeg->getSignedMin());

   auto negNeg = neg->negate();
   BOOST_REQUIRE(negNeg->isRegular());
   BOOST_REQUIRE_EQUAL(5, negNeg->getSignedMin());
   BOOST_REQUIRE_EQUAL(50, negNeg->getSignedMax());

   auto mixNeg = mix->negate();
   BOOST_REQUIRE(mixNeg->isRegular());
   BOOST_REQUIRE_EQUAL(50, mixNeg->getSignedMax());
   BOOST_REQUIRE_EQUAL(-37, mixNeg->getSignedMin());

   auto unsNeg = unsafe->negate();
   BOOST_REQUIRE(unsNeg->isAnti());
   BOOST_REQUIRE_EQUAL(-128, unsNeg->getSignedMin());
   auto unAnti = unsNeg->getAnti();
   BOOST_REQUIRE_EQUAL(-6, unAnti->getSignedMax());
   BOOST_REQUIRE_EQUAL(-127, unAnti->getSignedMin());

   auto aPosNeg = antiPos->negate();
   BOOST_REQUIRE(aPosNeg->isAnti());
   auto apnAnti = aPosNeg->getAnti();
   BOOST_REQUIRE_EQUAL(-6, apnAnti->getUpper());
   BOOST_REQUIRE_EQUAL(-10, apnAnti->getLower());

   auto aNegNeg = antiNeg->negate();
   BOOST_REQUIRE(aNegNeg->isAnti());
   auto annAnti = aNegNeg->getAnti();
   BOOST_REQUIRE_EQUAL(50, annAnti->getUpper());
   BOOST_REQUIRE_EQUAL(26, annAnti->getLower());

   auto aMixNeg = antiMix->negate();
   BOOST_REQUIRE(aMixNeg->isAnti());
   auto amnAnti = aMixNeg->getAnti();
   BOOST_REQUIRE_EQUAL(30, amnAnti->getUpper());
   BOOST_REQUIRE_EQUAL(-57, amnAnti->getLower());
}

BOOST_AUTO_TEST_CASE(range_and)
{
   RangeRef aPositive(new Range(Regular, 8, 0b00001010, 0b00010100));
   RangeRef aNegative(new Range(Regular, 8, 0b11000001, 0b11110000));
   RangeRef aMix(new Range(Regular, 8, 0b10101000, 0b00001101));
   RangeRef zero(new Range(Regular, 8, 0b00000000, 0b00000000));
   RangeRef bPositive(new Range(Regular, 8, 0b00000111, 0b00011011));
   RangeRef bNegative(new Range(Regular, 8, 0b10000000, 0b10000000));
   RangeRef bMix(new Range(Regular, 8, 0b10000000, 0b00000000));
   RangeRef one(new Range(Regular, 8, 0b00000001, 0b00000001));
   RangeRef allOnes(new Range(Regular, 8, 0b11111111, 0b11111111));

   auto andPaPb = aPositive->And(bPositive);
   auto andPbPa = bPositive->And(aPositive);
   BOOST_REQUIRE(andPaPb->isSameRange(andPbPa));
   BOOST_REQUIRE_EQUAL(0b00000000, andPaPb->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(0b00010100, andPaPb->getUnsignedMax());

   auto andPaZero = aPositive->And(zero);
   BOOST_REQUIRE_EQUAL(0, andPaZero->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(0, andPaZero->getUnsignedMin());

   auto andPaOne = aPositive->And(one);
   BOOST_REQUIRE_EQUAL(1, andPaOne->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(0, andPaOne->getUnsignedMin());

   auto andPaAllOnes = aPositive->And(allOnes);
   BOOST_REQUIRE_EQUAL(aPositive->getUnsignedMax(), andPaAllOnes->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(aPositive->getUnsignedMin(), andPaAllOnes->getUnsignedMin());

   RangeRef bOne(new Range(Regular, 1, 1, 1));
   RangeRef bZero(new Range(Regular, 1, 0, 0));
   RangeRef bOneZero(new Range(Regular, 1));

   auto orOneZero = bOne->And(bZero);
   BOOST_REQUIRE(orOneZero->isConstant());
   BOOST_REQUIRE_EQUAL(0, orOneZero->getUnsignedMax());

   auto orOneOneZero = bOne->And(bOneZero);
   BOOST_REQUIRE_EQUAL(0, orOneOneZero->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(1, orOneOneZero->getUnsignedMax());

   auto orZeroOneZero = bOneZero->And(bZero);
   BOOST_REQUIRE(orZeroOneZero->isConstant());
   BOOST_REQUIRE_EQUAL(0, orZeroOneZero->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE(range_or)
{
   RangeRef aPositive(new Range(Regular, 8, 0b00001010, 0b00010100));
   RangeRef aNegative(new Range(Regular, 8, 0b11000001, 0b11110000));
   RangeRef aMix(new Range(Regular, 8, 0b10101000, 0b00001101));
   RangeRef zero(new Range(Regular, 8, 0b00000000, 0b00000000));
   RangeRef bPositive(new Range(Regular, 8, 0b00000111, 0b00011011));
   RangeRef bNegative(new Range(Regular, 8, 0b10000000, 0b10000000));
   RangeRef bMix(new Range(Regular, 8, 0b10000000, 0b00000000));
   RangeRef allOnes(new Range(Regular, 8, 0b11111111, 0b11111111));

   auto orPaPb = aPositive->Or(bPositive);
   auto orPbPa = bPositive->Or(aPositive);
   BOOST_REQUIRE(orPaPb->isSameRange(orPbPa));
   BOOST_REQUIRE_EQUAL(0b00001010, orPaPb->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(0b00011111, orPaPb->getUnsignedMax());

   auto orPaZero = aPositive->Or(zero);
   BOOST_REQUIRE_EQUAL(aPositive->getUnsignedMin(), orPaZero->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(aPositive->getUnsignedMax(), orPaZero->getUnsignedMax());

   auto orPaAllOnes = aPositive->Or(allOnes);
   BOOST_REQUIRE_EQUAL(255, orPaAllOnes->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(255, orPaAllOnes->getUnsignedMax());

   RangeRef bOne(new Range(Regular, 1, 1, 1));
   RangeRef bZero(new Range(Regular, 1, 0, 0));
   RangeRef bOneZero(new Range(Regular, 1));

   auto orOneZero = bOne->Or(bZero);
   auto orZeroOne = bZero->Or(bOne);
   BOOST_REQUIRE(orOneZero->isSameRange(orZeroOne));
   BOOST_REQUIRE(orOneZero->isConstant());
   BOOST_REQUIRE_EQUAL(1, orOneZero->getUnsignedMax());

   auto orOneOneZero = bOne->Or(bOneZero);
   auto orOneZeroOne = bOneZero->Or(bOne);
   BOOST_REQUIRE(orOneOneZero->isSameRange(orOneZeroOne));
   BOOST_REQUIRE(orOneOneZero->isConstant());
   BOOST_REQUIRE_EQUAL(1, orOneOneZero->getUnsignedMax());

   auto orZeroOneZero = bOneZero->Or(bZero);
   auto orOneZeroZero = bZero->Or(bOneZero);
   BOOST_REQUIRE(orZeroOneZero->isSameRange(orOneZeroZero));
   BOOST_REQUIRE_EQUAL(0, orZeroOneZero->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(1, orZeroOneZero->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE(range_xor)
{
   RangeRef aPositive(new Range(Regular, 8, 0b00001010, 0b00010100));
   RangeRef aNegative(new Range(Regular, 8, 0b11000001, 0b11110000));
   RangeRef aMix(new Range(Regular, 8, 0b10101000, 0b00001101));
   RangeRef aConstant(new Range(Regular, 8, 0b00000000, 0b00000000));
   RangeRef bPositive(new Range(Regular, 8, 0b00000111, 0b00011011));
   RangeRef bNegative(new Range(Regular, 8, 0b10000000, 0b10000000));
   RangeRef bMix(new Range(Regular, 8, 0b10000000, 0b00000000));
   RangeRef bConstant(new Range(Regular, 8, 0b11111111, 0b11111111));

   auto xorPaPb = aPositive->Xor(bPositive);
   auto xorPbPa = bPositive->Xor(aPositive);
   BOOST_REQUIRE(xorPaPb->isSameRange(xorPbPa));
   BOOST_REQUIRE_EQUAL(0b00000000, xorPaPb->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(0b00011111, xorPaPb->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE(range_not)
{
   RangeRef pos(new Range(Regular, 8, 0b00001010, 0b00010100));
   RangeRef neg(new Range(Regular, 8, 0b11000001, 0b11110000));
   RangeRef mix(new Range(Regular, 8, 0b10101000, 0b00001101));
   RangeRef anti(new Range(Anti, 8, static_cast<int8_t>(0b10101000), 0b00001101));

   auto notPos = pos->Not();
   BOOST_REQUIRE_EQUAL(0b11101011, notPos->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(0b11110101, notPos->getUnsignedMax());

   auto notNeg = neg->Not();
   BOOST_REQUIRE_EQUAL(0b00001111, notNeg->getSignedMin());
   BOOST_REQUIRE_EQUAL(0b00111110, notNeg->getSignedMax());

   auto notMix = mix->Not();
   BOOST_REQUIRE_EQUAL(static_cast<int8_t>(0b11110010), notMix->getSignedMin());
   BOOST_REQUIRE_EQUAL(0b01010111, notMix->getSignedMax());

   auto notAnti = anti->Not();
   BOOST_REQUIRE_EQUAL(0b11110001, notAnti->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE(range_lt)
{
   RangeRef one(new Range(Regular, 1, 1, 1));
   RangeRef zero(new Range(Regular, 1, 0, 0));
   RangeRef zeroOne(new Range(Regular, 1));

   auto ltOneZero = one->Slt(zero, 1);
   BOOST_REQUIRE(ltOneZero->isConstant());
   BOOST_REQUIRE(one->isSameRange(ltOneZero));

   auto ltZeroOneOne = zeroOne->Slt(one, 1);
   BOOST_REQUIRE(ltZeroOneOne->isConstant());
   BOOST_REQUIRE(zero->isSameRange(ltZeroOneOne));

   auto ultOneZero = one->Ult(zero, 1);
   BOOST_REQUIRE(ultOneZero->isConstant());
   BOOST_REQUIRE(zero->isSameRange(ultOneZero));

   auto ultZeroOneOne = zeroOne->Ult(one, 1);
   BOOST_REQUIRE(zeroOne->isSameRange(ultZeroOneOne));
}

BOOST_AUTO_TEST_CASE(range_le)
{
   RangeRef one(new Range(Regular, 1, 1, 1));
   RangeRef zero(new Range(Regular, 1, 0, 0));
   RangeRef zeroOne(new Range(Regular, 1));

   auto leOneZero = one->Sle(zero, 1);
   BOOST_REQUIRE(leOneZero->isConstant());
   BOOST_REQUIRE(one->isSameRange(leOneZero));

   auto leZeroOneOne = zeroOne->Sle(one, 1);
   BOOST_REQUIRE(zeroOne->isSameRange(leZeroOneOne));

   auto uleOneZero = one->Ule(zero, 1);
   BOOST_REQUIRE(uleOneZero->isConstant());
   BOOST_REQUIRE(zero->isSameRange(uleOneZero));

   auto uleZeroOneOne = zeroOne->Ule(one, 1);
   BOOST_REQUIRE(uleZeroOneOne->isConstant());
   BOOST_REQUIRE(one->isSameRange(uleZeroOneOne));
}

BOOST_AUTO_TEST_CASE(range_gt)
{
   RangeRef one(new Range(Regular, 1, 1, 1));
   RangeRef zero(new Range(Regular, 1, 0, 0));
   RangeRef zeroOne(new Range(Regular, 1));

   // When 1bit integer is signed 1 is actually -1
   auto gtOneZero = one->Sgt(zero, 1);
   BOOST_REQUIRE(gtOneZero->isConstant());
   BOOST_REQUIRE(zero->isSameRange(gtOneZero));

   auto gtZeroOneOne = zeroOne->Sgt(one, 1);
   BOOST_REQUIRE(zeroOne->isSameRange(gtZeroOneOne));

   auto ugtOneZero = one->Ugt(zero, 1);
   BOOST_REQUIRE(ugtOneZero->isConstant());
   BOOST_REQUIRE(one->isSameRange(ugtOneZero));

   auto ugtZeroOneOne = zeroOne->Ugt(one, 1);
   BOOST_REQUIRE(ugtZeroOneOne->isConstant());
   BOOST_REQUIRE(zero->isSameRange(ugtZeroOneOne));
}

BOOST_AUTO_TEST_CASE(range_ge)
{
   RangeRef one(new Range(Regular, 1, 1, 1));
   RangeRef zero(new Range(Regular, 1, 0, 0));
   RangeRef zeroOne(new Range(Regular, 1));

   auto geOneZero = one->Sge(zero, 1);
   BOOST_REQUIRE(geOneZero->isConstant());
   BOOST_REQUIRE(zero->isSameRange(geOneZero));

   auto geZeroOneOne = zeroOne->Sge(one, 1);
   BOOST_REQUIRE(geZeroOneOne->isConstant());
   BOOST_REQUIRE(one->isSameRange(geZeroOneOne));

   auto ugeOneZero = one->Uge(zero, 1);
   BOOST_REQUIRE(ugeOneZero->isConstant());
   BOOST_REQUIRE(one->isSameRange(ugeOneZero));

   auto ugeZeroOneOne = zeroOne->Uge(one, 1);
   BOOST_REQUIRE(zeroOne->isSameRange(ugeZeroOneOne));
}

BOOST_AUTO_TEST_CASE(range_truncate)
{
   RangeRef contained(new Range(Regular, 8, 0b11111001, 0b00001010));
   RangeRef lowwrap(new Range(Regular, 8, 0b11101101, 0b00001010));
   RangeRef upwrap(new Range(Regular, 8, 0b11111111, 0b00011000));
   RangeRef fullwrap(new Range(Regular, 8, 0b11100111, 0b11101111));
   RangeRef fullwrap2(new Range(Regular, 16, -3786, -3703));
   RangeRef multiwrap(new Range(Regular, 16, 1250, 2277));

   auto truncContained = contained->truncate(5);
   BOOST_REQUIRE(truncContained->isRegular());
   BOOST_REQUIRE_EQUAL(contained->getSignedMax(), truncContained->getSignedMax());
   BOOST_REQUIRE_EQUAL(-7, contained->getSignedMin());

   auto truncLowwrap = lowwrap->truncate(5);
   BOOST_REQUIRE(truncLowwrap->isAnti());

   auto truncUpwrap = upwrap->truncate(5);
   BOOST_REQUIRE(truncUpwrap->isAnti());

   auto truncFullwrap = fullwrap->truncate(5);
   BOOST_REQUIRE(truncFullwrap->isRegular());
   BOOST_REQUIRE_EQUAL(0b00000111, truncFullwrap->getSignedMin());
   BOOST_REQUIRE_EQUAL(0b00001111, truncFullwrap->getSignedMax());

   auto truncFullwrap2 = fullwrap2->truncate(8);
   BOOST_REQUIRE_EQUAL(-119, truncFullwrap2->getSignedMin());
   BOOST_REQUIRE_EQUAL(54, truncFullwrap2->getSignedMax());

   auto truncMultiwrap = multiwrap->truncate(8);
   BOOST_REQUIRE(truncMultiwrap->isFullSet());

   RangeRef strangeTest1(new Range(Regular, 32, 122, 124));
   auto strangeRes1 = strangeTest1->truncate(7);
   BOOST_REQUIRE_EQUAL(122, strangeRes1->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(-6, strangeRes1->getSignedMin());
   BOOST_REQUIRE_EQUAL(124, strangeRes1->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(-4, strangeRes1->getSignedMax());
   strangeRes1 = strangeRes1->zextOrTrunc(32);
   BOOST_REQUIRE_EQUAL(122, strangeRes1->getSignedMin());
   BOOST_REQUIRE_EQUAL(124, strangeRes1->getSignedMax());
   BOOST_REQUIRE_EQUAL(strangeRes1->getUnsignedMin(), strangeRes1->getSignedMin());
   BOOST_REQUIRE_EQUAL(strangeRes1->getUnsignedMax(), strangeRes1->getSignedMax());

   RangeRef test1(new Range(Regular, 8, -33, -30));
   auto trunc5 = test1->truncate(5);
   BOOST_REQUIRE_EQUAL(-1, trunc5->getSignedMin());
   BOOST_REQUIRE_EQUAL(2, trunc5->getSignedMax());
}

BOOST_AUTO_TEST_CASE(range_zext)
{
   RangeRef boolean(new Range(Regular, 1));
   RangeRef signedR(new Range(Regular, 8, 0b11001100, 0b01110011));
   RangeRef unsignedR(new Range(Regular, 8, 0b00011100, 0b01100101));
   RangeRef empty(new Range(Empty, 2));
   RangeRef unknown(new Range(Unknown, 2));

   auto boolToChar = boolean->zextOrTrunc(8);
   BOOST_REQUIRE_EQUAL(8, boolToChar->getBitWidth());
   BOOST_REQUIRE_EQUAL(0, boolToChar->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(1, boolToChar->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(boolToChar->getUnsignedMin(), boolToChar->getSignedMin());
   BOOST_REQUIRE_EQUAL(boolToChar->getUnsignedMax(), boolToChar->getSignedMax());

   auto charSToShort = signedR->zextOrTrunc(16);
   BOOST_REQUIRE_EQUAL(16, charSToShort->getBitWidth());
   BOOST_REQUIRE_EQUAL(255, charSToShort->getSignedMax());
   BOOST_REQUIRE_EQUAL(0, charSToShort->getSignedMin());

   auto charUToShort = unsignedR->zextOrTrunc(16);
   BOOST_REQUIRE_EQUAL(16, charUToShort->getBitWidth());
   BOOST_REQUIRE_EQUAL(unsignedR->getUnsignedMax(), charUToShort->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(unsignedR->getUnsignedMin(), charUToShort->getUnsignedMin());

   auto emptyZext = empty->zextOrTrunc(5);
   BOOST_REQUIRE_EQUAL(5, emptyZext->getBitWidth());
   BOOST_REQUIRE(emptyZext->isEmpty());

   auto unknownZext = unknown->zextOrTrunc(5);
   BOOST_REQUIRE_EQUAL(5, unknownZext->getBitWidth());
   BOOST_REQUIRE(unknownZext->isUnknown());
}

BOOST_AUTO_TEST_CASE(range_sext)
{
   RangeRef boolean(new Range(Regular, 1));
   RangeRef signedR(new Range(Regular, 8, 0b11001100, 0b01110011));
   RangeRef unsignedR(new Range(Regular, 8, 0b00011100, 0b01100101));
   RangeRef empty(new Range(Empty, 2));
   RangeRef unknown(new Range(Unknown, 2));

   auto boolToChar = boolean->sextOrTrunc(8);
   BOOST_REQUIRE_EQUAL(8, boolToChar->getBitWidth());
   BOOST_REQUIRE_EQUAL(-1, boolToChar->getSignedMin());
   BOOST_REQUIRE_EQUAL(0, boolToChar->getSignedMax());

   auto charSToShort = signedR->sextOrTrunc(16);
   BOOST_REQUIRE_EQUAL(16, charSToShort->getBitWidth());
   BOOST_REQUIRE_EQUAL(signedR->getSignedMin(), charSToShort->getSignedMin());
   BOOST_REQUIRE_EQUAL(signedR->getSignedMax(), charSToShort->getSignedMax());

   auto charUToShort = unsignedR->sextOrTrunc(16);
   BOOST_REQUIRE_EQUAL(16, charUToShort->getBitWidth());
   BOOST_REQUIRE_EQUAL(unsignedR->getUnsignedMin(), charUToShort->getSignedMin());
   BOOST_REQUIRE_EQUAL(unsignedR->getUnsignedMax(), charUToShort->getSignedMax());

   auto emptySext = empty->sextOrTrunc(5);
   BOOST_REQUIRE_EQUAL(5, emptySext->getBitWidth());
   BOOST_REQUIRE(emptySext->isEmpty());

   auto unknownSext = unknown->sextOrTrunc(5);
   BOOST_REQUIRE_EQUAL(5, unknownSext->getBitWidth());
   BOOST_REQUIRE(unknownSext->isUnknown());
}

BOOST_AUTO_TEST_CASE(real_range)
{
   RangeRef stdFullRange(new Range(Regular, 32));
   RangeRef stdFullRange64(new Range(Regular, 64));
   RealRange constFloat64(Range(Regular, 1, 0, 0), Range(Regular, 11, 0b10000000001, 0b10000000001), Range(Regular, 52, 0b1110000000000000000000000000000000000000000000000000, 0b1110000000000000000000000000000000000000000000000000));
   RealRange constDouble(Range(Regular, 1, 0, 0), Range(Regular, 11, 0b10100000001, 0b10100000001), Range(Regular, 52, 0b1110000000000000000000000000000000000000000000000000, 0b1110000000000000000000000000000000000000000000000000));
   RealRange constFloat32(Range(Regular, 1, 0, 0), Range(Regular, 8, 0b10000001, 0b10000001), Range(Regular, 23, 0b11100000000000000000000, 0b11100000000000000000000));

   RealRange float64(stdFullRange64);
   BOOST_REQUIRE(float64.getSign()->isFullSet());
   BOOST_REQUIRE(float64.getExponent()->isFullSet());
   BOOST_REQUIRE(float64.getSignificand()->isFullSet());

   RealRange float32(stdFullRange);
   BOOST_REQUIRE(float32.getSign()->isFullSet());
   BOOST_REQUIRE(float32.getExponent()->isFullSet());
   BOOST_REQUIRE(float32.getSignificand()->isFullSet());

   auto view_convert32 = float32.getRange();
   BOOST_REQUIRE_EQUAL(32, view_convert32->getBitWidth());
   BOOST_REQUIRE(view_convert32->isFullSet());

   auto view_convert64 = float64.getRange();
   BOOST_REQUIRE_EQUAL(64, view_convert64->getBitWidth());
   BOOST_REQUIRE(view_convert64->isFullSet());

   auto cast32to64 = RefcountCast<const RealRange>(float32.toFloat64());
   BOOST_REQUIRE_EQUAL(64, cast32to64->getBitWidth());
   BOOST_REQUIRE(!cast32to64->isFullSet());
   BOOST_REQUIRE(cast32to64->getExponent()->isFullSet());
   BOOST_REQUIRE(cast32to64->getSign()->isFullSet());

   auto cast64to32 = float64.toFloat32();
   BOOST_REQUIRE_EQUAL(32, cast64to32->getBitWidth());
   BOOST_REQUIRE(cast64to32->isFullSet());

   auto const64To32 = constFloat64.toFloat32();
   auto const32To64 = constFloat32.toFloat64();
   BOOST_REQUIRE(constFloat64.isSameRange(const32To64));
   BOOST_REQUIRE(constFloat32.isSameRange(const64To32));

   auto constDoubleToFloat = RefcountCast<const RealRange>(constDouble.toFloat32());
   BOOST_REQUIRE(constDoubleToFloat->isConstant());
   BOOST_REQUIRE_EQUAL(0, constDoubleToFloat->getSign()->getUnsignedMin());
   BOOST_REQUIRE_EQUAL(0b11111111, constDoubleToFloat->getExponent()->getUnsignedMax());
   BOOST_REQUIRE_EQUAL(0, constDoubleToFloat->getSignificand()->getUnsignedMin());

   auto constFloat32VC = constFloat32.getRange();
   BOOST_REQUIRE(constFloat32VC->isConstant());
   BOOST_REQUIRE_EQUAL(0b01000000111100000000000000000000, constFloat32VC->getUnsignedMin());

   auto constFloat64VC = constFloat64.getRange();
   BOOST_REQUIRE(constFloat64VC->isConstant());
   BOOST_REQUIRE_EQUAL(0b0100000000011110000000000000000000000000000000000000000000000000, constFloat64VC->getUnsignedMin());

   auto floatNegOne = RealRange(RangeRef(new Range(Regular, 64, 13830554455654793216ULL, 13830554455654793216ULL)));
   BOOST_REQUIRE(floatNegOne.isConstant());
   BOOST_REQUIRE_EQUAL(floatNegOne.getSign()->getUnsignedMax(), 1);
   auto floatOne = RealRange(RangeRef(new Range(Regular, 64, 4607182418800017408ULL, 4607182418800017408ULL)));
   BOOST_REQUIRE(floatOne.isConstant());
   BOOST_REQUIRE_EQUAL(floatOne.getSign()->getUnsignedMax(), 0);
}

BOOST_AUTO_TEST_CASE(real_eq)
{
   RangeRef stdFullRange(new Range(Regular, 32));
   RangeRef stdFullRange64(new Range(Regular, 64));
   RangeRef pzerod(new RealRange(Range(Regular, 1, 0, 0), Range(Regular, 11, 0, 0), Range(Regular, 52, 0, 0)));
   RangeRef nzerod(new RealRange(Range(Regular, 1, 1, 1), Range(Regular, 11, 0, 0), Range(Regular, 52, 0, 0)));
   RangeRef negWithZero(new RealRange(Range(Regular, 1, 1, 1), Range(Regular, 11, 0, 1035), Range(Regular, 52)));
   RangeRef noZero(new RealRange(Range(Regular, 1, 0, 1), Range(Regular, 11, 997, 1035), Range(Regular, 52)));

   auto eq1 = pzerod->Eq(nzerod, 1);
   auto ne1 = pzerod->Ne(nzerod, 1);
   BOOST_REQUIRE(eq1->isConstant());
   BOOST_REQUIRE_EQUAL(eq1->getUnsignedMin(), 1);
   BOOST_REQUIRE(ne1->isConstant());
   BOOST_REQUIRE_EQUAL(ne1->getUnsignedMin(), 0);

   auto eq2 = negWithZero->Eq(nzerod, 1);
   auto ne2 = negWithZero->Ne(nzerod, 1);
   BOOST_REQUIRE(eq2->isSameRange(ne2));
   BOOST_REQUIRE_EQUAL(eq2->getUnsignedMin(), 0);
   BOOST_REQUIRE_EQUAL(eq2->getUnsignedMax(), 1);

   auto eq3 = negWithZero->Eq(pzerod, 1);
   auto ne3 = negWithZero->Ne(pzerod, 1);
   BOOST_REQUIRE(eq3->isSameRange(ne3));
   BOOST_REQUIRE_EQUAL(eq3->getUnsignedMin(), 0);
   BOOST_REQUIRE_EQUAL(eq3->getUnsignedMax(), 1);

   auto eq4 = noZero->Eq(pzerod, 1);
   auto ne4 = noZero->Ne(pzerod, 1);
   BOOST_REQUIRE(eq4->isConstant());
   BOOST_REQUIRE_EQUAL(eq4->getUnsignedMax(), 0);
   BOOST_REQUIRE(ne4->isConstant());
   BOOST_REQUIRE_EQUAL(ne4->getUnsignedMin(), 1);

   auto eq5 = noZero->Eq(nzerod, 1);
   auto ne5 = noZero->Ne(nzerod, 1);
   BOOST_REQUIRE(eq5->isConstant());
   BOOST_REQUIRE_EQUAL(eq5->getUnsignedMax(), 0);
   BOOST_REQUIRE(ne5->isConstant());
   BOOST_REQUIRE_EQUAL(ne5->getUnsignedMin(), 1);
}