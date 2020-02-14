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
    BOOST_REQUIRE(aPositive->isSameRange(sdivPaI));

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

BOOST_AUTO_TEST_CASE( range_shl )
{
    RangeRef aPositive(new Range(Regular, 8, 0b00000011, 0b00000101));
    RangeRef aNegative(new Range(Regular, 8, 0b11000001, 0b11110000));
    RangeRef aMix(new Range(Regular, 8, 0b10101000, 0b00001101));
    RangeRef zero(new Range(Regular, 8, 0, 0));
    RangeRef bPositive(new Range(Regular, 8, 0b00000111, 0b00011011));
    RangeRef bNegative(new Range(Regular, 8, 0b10000000, 0b10000000));
    RangeRef bMix(new Range(Regular, 8, 0b10000000, 0));
    RangeRef one(new Range(Regular, 8, 1, 1));
    RangeRef zeroOne(new Range(Regular, 8, 0, 1));
    RangeRef allOnes(new Range(Regular, 8, 0b11111111, 0b11111111));
    RangeRef seven(new Range(Regular, 8, 7, 7));

    auto shlAllOnes = allOnes->shl(aPositive);
    BOOST_REQUIRE_EQUAL(0b11111000, shlAllOnes->getUnsignedMax());
    BOOST_REQUIRE_EQUAL(0b11100000, shlAllOnes->getUnsignedMin());

    auto shlZeroOneSeven = zeroOne->shl(seven);
    BOOST_REQUIRE_EQUAL(0b10000000, shlZeroOneSeven->getUnsignedMax());
    BOOST_REQUIRE_EQUAL(0, shlZeroOneSeven->getUnsignedMin());

    auto shlOneSeven = one->shl(seven);
    BOOST_REQUIRE(shlOneSeven->isConstant());
    BOOST_REQUIRE_EQUAL(0b10000000, shlOneSeven->getUnsignedMin());
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
    BOOST_REQUIRE(xorPaPb->isSameRange(xorPbPa));
    BOOST_REQUIRE_EQUAL(0b00000000, xorPaPb->getUnsignedMin());
    BOOST_REQUIRE_EQUAL(0b00011111, xorPaPb->getUnsignedMax());
}

BOOST_AUTO_TEST_CASE( range_not )
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

BOOST_AUTO_TEST_CASE( range_lt )
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

BOOST_AUTO_TEST_CASE( range_le )
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

BOOST_AUTO_TEST_CASE( range_gt )
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

BOOST_AUTO_TEST_CASE( range_ge )
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

BOOST_AUTO_TEST_CASE( range_truncate )
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
    BOOST_REQUIRE(truncFullwrap2->isAnti());

    auto truncMultiwrap = multiwrap->truncate(8);
    BOOST_REQUIRE(truncMultiwrap->isFullSet());
}

BOOST_AUTO_TEST_CASE( range_zext )
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

BOOST_AUTO_TEST_CASE( range_sext )
{
    RangeRef boolean(new Range(Regular, 1));
    RangeRef signedR(new Range(Regular, 8, 0b11001100, 0b01110011));
    RangeRef unsignedR(new Range(Regular, 8, 0b00011100, 0b01100101));
    RangeRef empty(new Range(Empty, 2));
    RangeRef unknown(new Range(Unknown, 2));

    auto boolToChar = boolean->sextOrTrunc(8);
    BOOST_REQUIRE_EQUAL(8, boolToChar->getBitWidth());
    BOOST_REQUIRE_EQUAL(0, boolToChar->getSignedMin());
    BOOST_REQUIRE_EQUAL(1, boolToChar->getSignedMax());

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

BOOST_AUTO_TEST_CASE( real_range )
{
    RangeRef stdFullRange(new Range(Regular, 32));
    RangeRef stdFullRange64(new Range(Regular, 64));
    RealRange constFloat64(Range(Regular, 1, 0, 0), Range(Regular, 11, 0b10000000001, 0b10000000001), Range(Regular, 52, 0b1110000000000000000000000000000000000000000000000000, 0b1110000000000000000000000000000000000000000000000000));
    RealRange constDouble(Range(Regular, 1, 0, 0), Range(Regular, 11, 0b10100000001, 0b10100000001), Range(Regular, 52, 0b1110000000000000000000000000000000000000000000000000, 0b1110000000000000000000000000000000000000000000000000));
    RealRange constFloat32(Range(Regular, 1, 0, 0), Range(Regular, 8, 0b10000001, 0b10000001), Range(Regular, 23, 0b11100000000000000000000, 0b11100000000000000000000));

    RealRange float64(stdFullRange64);
    BOOST_REQUIRE(float64.getSign()->isFullSet());
    BOOST_REQUIRE(float64.getExponent()->isFullSet());
    BOOST_REQUIRE(float64.getFractional()->isFullSet());
    
    RealRange float32(stdFullRange);
    BOOST_REQUIRE(float32.getSign()->isFullSet());
    BOOST_REQUIRE(float32.getExponent()->isFullSet());
    BOOST_REQUIRE(float32.getFractional()->isFullSet());

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
    BOOST_REQUIRE_EQUAL(0, constDoubleToFloat->getFractional()->getUnsignedMin());
}