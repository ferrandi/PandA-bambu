// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
// The allocation step characterizes vector operations with the element width of
// the functional unit it looks up, and with the lane count derived from that
// width. The resource library and the generated components only exist for 1-bit
// and for byte-or-larger power-of-two element widths, so a logical width such as
// i7 must be rounded up for the lookup while the lane counts stay logical. A
// vector whose element width is neither 1 nor a byte-aligned power of two fails
// to allocate when the logical width is used as the lookup key.

#include "ir_helper.hpp"
#include "ir_node.hpp"

#include <boost/test/unit_test.hpp>

#include <memory>

namespace
{
   ir_nodeRef make_vector(unsigned element_bitsize, unsigned long long lanes)
   {
      auto element = std::make_shared<integer_ty_node>(1);
      element->bitsize = element_bitsize;
      element->bitsizealloc = (element_bitsize + 7) / 8 * 8;
      element->algn = element->bitsizealloc;

      auto vector = std::make_shared<vector_ty_node>(2);
      vector->elts = element;
      vector->nelements = lanes;
      vector->bitsizealloc = static_cast<unsigned>(element->bitsizealloc * lanes);
      return vector;
   }
} // namespace

BOOST_AUTO_TEST_CASE(vector_allocation_keeps_logical_element_width)
{
   const auto i7 = make_vector(7, 4);
   BOOST_CHECK_EQUAL(ir_helper::VectorElementBitsize(i7), 7);
   BOOST_CHECK_EQUAL(ir_helper::VectorNumElements(i7), 4);
   BOOST_CHECK_EQUAL(ir_helper::VectorLogicalBitsize(i7), 28);
}

BOOST_AUTO_TEST_CASE(vector_allocation_characterizes_element_width)
{
   // Single-bit lanes keep their width: the library has dedicated one-bit vector
   // components, and rounding them up would select the wrong one.
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(1, 8)), 1);

   // Byte-or-larger power-of-two widths are used as they are.
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(8, 16)), 8);
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(16, 8)), 16);
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(32, 4)), 32);
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(64, 2)), 64);

   // Anything else is rounded up to the next byte-aligned power of two, which is
   // the width the characterized component and its HDL wrapper are built for.
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(2, 4)), 8);
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(7, 4)), 8);
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(12, 4)), 16);
   BOOST_CHECK_EQUAL(ir_helper::VectorCharacterizedElementBitsize(make_vector(17, 4)), 32);

   // The lookup width is what the 128-bit vector components are divided by, so
   // the lane count of the characterization follows it and never becomes zero.
   for(const unsigned width : {1U, 2U, 7U, 12U, 17U, 32U})
   {
      const auto vector = make_vector(width, 4);
      const auto characterized = ir_helper::VectorCharacterizedElementBitsize(vector);
      BOOST_CHECK(characterized >= width);
      BOOST_CHECK(characterized != 0 && 128 / characterized != 0);
      BOOST_CHECK((characterized & (characterized - 1)) == 0);
   }
}
