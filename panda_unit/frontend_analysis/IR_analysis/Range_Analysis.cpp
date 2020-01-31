#include "Range_Analysis.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE( range )
{
    RangeRef r;
    BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8 ,-5, 113)));
    BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
    BOOST_REQUIRE_EQUAL(0, r->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(255, r->getUnsignedMax());
    BOOST_REQUIRE_EQUAL(-5, r->getSignedMin());
    BOOST_REQUIRE_EQUAL(113, r->getSignedMax());
    BOOST_REQUIRE(!r->isFullSet());

    BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8)));
    BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
    BOOST_REQUIRE_EQUAL(0, r->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(255, r->getUnsignedMax());
    BOOST_REQUIRE_EQUAL(-128, r->getSignedMin());
    BOOST_REQUIRE_EQUAL(127, r->getSignedMax());
    BOOST_REQUIRE(r->isFullSet());

    BOOST_REQUIRE_NO_THROW(r.reset(new Range(Regular, 8, -3, 257)));
    BOOST_REQUIRE_EQUAL(8, r->getBitWidth());
    BOOST_REQUIRE_EQUAL(0, r->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(255, r->getUnsignedMax());
    BOOST_REQUIRE_EQUAL(-128, r->getSignedMin());
    BOOST_REQUIRE_EQUAL(127, r->getSignedMax());
    BOOST_REQUIRE(r->isFullSet());
}

BOOST_AUTO_TEST_CASE( range_addition )
{
    RangeRef aPositive(new Range(Regular, 8, 5, 9));
    RangeRef aNegative(new Range(Regular, 8, -9, -5));
    RangeRef aMix(new Range(Regular, 8, -5, 9));
    RangeRef bPositive(new Range(Regular, 8, 3, 6));
    RangeRef bNegative(new Range(Regular, 8, -6, -3));
    RangeRef bMix(new Range(Regular, 8, -3, 6));

    auto addPaPb = aPositive->add(bPositive);
    auto addPbPa = bPositive->add(aPositive);
    BOOST_REQUIRE(addPaPb->operator==(*addPbPa));
    BOOST_REQUIRE_EQUAL(addPaPb->getBitWidth(), aPositive->getBitWidth());
    BOOST_REQUIRE_EQUAL(15, addPaPb->getSignedMax());
    BOOST_REQUIRE_EQUAL(8, addPaPb->getSignedMin());

    auto addNaNb = aNegative->add(bNegative);
    auto addNbNa = bNegative->add(aNegative);
    BOOST_REQUIRE(addNaNb->operator==(*addNbNa));
    BOOST_REQUIRE_EQUAL(-15, addNaNb->getSignedMin());
    BOOST_REQUIRE_EQUAL(-8, addNaNb->getSignedMax());

    auto addMaMb = aMix->add(bMix);
    auto addMbMa = bMix->add(aMix);
    BOOST_REQUIRE(addMaMb->operator==(*addMbMa));
    BOOST_REQUIRE_EQUAL(15, addMaMb->getSignedMax());
    BOOST_REQUIRE_EQUAL(-8, addMaMb->getSignedMin());

    auto addPaNb = aPositive->add(bNegative);
    auto addNbPa = bNegative->add(aPositive);
    BOOST_REQUIRE(addPaNb->operator==(*addNbPa));
    BOOST_REQUIRE_EQUAL(-1, addPaNb->getSignedMin());
    BOOST_REQUIRE_EQUAL(6, addPaNb->getSignedMax());
}

BOOST_AUTO_TEST_CASE( range_subtraction )
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

BOOST_AUTO_TEST_CASE( range_multiplication )
{
    RangeRef aPositive(new Range(Regular, 8, 5, 9));
    RangeRef aNegative(new Range(Regular, 8, -9, -5));
    RangeRef aMix(new Range(Regular, 8, -5, 9));
    RangeRef bPositive(new Range(Regular, 8, 3, 6));
    RangeRef bNegative(new Range(Regular, 8, -6, -3));
    RangeRef bMix(new Range(Regular, 8, -3, 6));

    auto mulPaPb = aPositive->mul(bPositive);
    auto mulPbPa = bPositive->mul(aPositive);
    BOOST_REQUIRE(mulPaPb->operator==(*mulPbPa));
    BOOST_REQUIRE_EQUAL(mulPaPb->getBitWidth(), aPositive->getBitWidth());
    BOOST_REQUIRE_EQUAL(54, mulPaPb->getSignedMax());
    BOOST_REQUIRE_EQUAL(15, mulPaPb->getSignedMin());

    auto mulNaNb = aNegative->mul(bNegative);
    auto mulNbNa = bNegative->mul(aNegative);
    BOOST_REQUIRE(mulNaNb->operator==(*mulNbNa));
    BOOST_REQUIRE_EQUAL(15, mulNaNb->getSignedMin());
    BOOST_REQUIRE_EQUAL(54, mulNaNb->getSignedMax());

    auto mulMaMb = aMix->mul(bMix);
    auto mulMbMa = bMix->mul(aMix);
    BOOST_REQUIRE(mulMaMb->operator==(*mulMbMa));
    BOOST_REQUIRE_EQUAL(-30, mulMaMb->getSignedMin());
    BOOST_REQUIRE_EQUAL(54, mulMaMb->getSignedMax());

    auto mulPaNb = aPositive->mul(bNegative);
    auto mulNbPa = bNegative->mul(aPositive);
    BOOST_REQUIRE(mulPaNb->operator==(*mulNbPa));
    BOOST_REQUIRE_EQUAL(-54, mulPaNb->getSignedMin());
    BOOST_REQUIRE_EQUAL(-15, mulPaNb->getSignedMax());
}

BOOST_AUTO_TEST_CASE( range_division )
{
    RangeRef aPositive(new Range(Regular, 8, 5, 9));
    RangeRef aNegative(new Range(Regular, 8, -9, -5));
    RangeRef aMix(new Range(Regular, 8, -5, 9));
    RangeRef bPositive(new Range(Regular, 8, 3, 6));
    RangeRef bNegative(new Range(Regular, 8, -6, -3));
    RangeRef bMix(new Range(Regular, 8, -3, 6));

    RangeRef invariant(new Range(Regular, 8, 1, 1));

    auto sdivPaI = aPositive->sdiv(invariant);
    BOOST_REQUIRE(aPositive->operator==(*sdivPaI));

    auto sdivPaNb = aPositive->sdiv(bNegative);
    BOOST_REQUIRE_EQUAL(-3, sdivPaNb->getSignedMin());
    BOOST_REQUIRE_EQUAL(0, sdivPaNb->getSignedMax());

    auto udivPaNb = aPositive->udiv(bNegative);
    BOOST_REQUIRE_EQUAL(0, udivPaNb->getUnsignedMax());
    BOOST_REQUIRE_EQUAL(0, udivPaNb->getSignedMin());
}

BOOST_AUTO_TEST_CASE( range_reminder )
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

BOOST_AUTO_TEST_CASE( range_and )
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
    BOOST_REQUIRE(andPaPb->operator==(*andPbPa));
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
}

BOOST_AUTO_TEST_CASE( range_or )
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
    BOOST_REQUIRE(orPaPb->operator==(*orPbPa));
    BOOST_REQUIRE_EQUAL(0b00001010, orPaPb->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(0b00011111, orPaPb->getUnsignedMax());

    auto orPaZero = aPositive->Or(zero);
    BOOST_REQUIRE_EQUAL(aPositive->getUnsignedMin(), orPaZero->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(aPositive->getUnsignedMax(), orPaZero->getUnsignedMax());

    auto orPaAllOnes = aPositive->Or(allOnes);
    BOOST_REQUIRE_EQUAL(255, orPaAllOnes->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(255, orPaAllOnes->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE( range_xor )
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
    BOOST_REQUIRE(xorPaPb->operator==(*xorPbPa));
    BOOST_REQUIRE_EQUAL(0b00000000, xorPaPb->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(0b00011111, xorPaPb->getUnsignedMax());
}