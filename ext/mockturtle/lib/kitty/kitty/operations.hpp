/* kitty: C++ truth table library
 * Copyright (C) 2017-2020  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file operations.hpp
  \brief Implements several operations on truth tables

  \author Mathias Soeken
  \author Mahyar Emami (mahyar.emami@epfl.ch)
*/

#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <iterator>

#include "algorithm.hpp"
#include "dynamic_truth_table.hpp"
#include "static_truth_table.hpp"
#include "partial_truth_table.hpp"
#include "detail/shift.hpp"
#include "traits.hpp"

namespace kitty
{

/* forward declarations */
/*! \cond PRIVATE */
template<typename TT>
TT create( unsigned num_vars );

template<>
dynamic_truth_table create<dynamic_truth_table>( unsigned num_vars );
/*! \endcond */

/*! \brief Inverts all bits in a truth table */
template<typename TT>
inline TT unary_not( const TT& tt )
{
  return unary_operation( tt, []( auto a ) { return ~a; } );
}

/*! Inverts all bits in a truth table, based on a condition */
template<typename TT>
inline TT unary_not_if( const TT& tt, bool cond )
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4146)
#endif
  const auto mask = -static_cast<uint64_t>( cond );
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  return unary_operation( tt, [mask]( auto a ) { return a ^ mask; } );
}

/*! \brief Bitwise AND of two truth tables */
template<typename TT>

inline TT binary_and( const TT& first, const TT& second )
{
  return binary_operation( first, second, std::bit_and<>() );
}

/*! \brief Bitwise OR of two truth tables */
template<typename TT>
inline TT binary_or( const TT& first, const TT& second )
{
  return binary_operation( first, second, std::bit_or<>() );
}

/*! \brief Bitwise XOR of two truth tables */
template<typename TT>
inline TT binary_xor( const TT& first, const TT& second )
{
  return binary_operation( first, second, std::bit_xor<>() );
}

/*! \brief Ternary majority of three truth tables */
template<typename TT>
inline TT ternary_majority( const TT& first, const TT& second, const TT& third )
{
  return ternary_operation( first, second, third, []( auto a, auto b, auto c ) { return ( a & ( b ^ c ) ) ^ ( b & c ); } );
}

/*! \brief Performs ternary if-then-else of three truth tables

  \param first Truth table for condition
  \param second Truth table for then-case
  \param third Truth table for else-case
 */
template<typename TT>
inline TT ternary_ite( const TT& first, const TT& second, const TT& third )
{
  return ternary_operation( first, second, third, []( auto a, auto b, auto c ) { return ( a & b ) ^ ( ~a & c ); } );
}

/*! \brief Muxes two truth tables based on a variable

  \param var_index Variable index
  \param then_ Truth table for the then-case
  \param else_ Truth table for the else-case
*/
template<typename TT>
inline TT mux_var( uint8_t var_index, const TT& then_, const TT& else_ )
{
  if ( var_index < 6u )
  {
    return binary_operation( then_, else_,
                             [&]( auto a, auto b ) { return ( a & detail::projections[var_index] ) |
                                                            ( b & detail::projections_neg[var_index] ); } );
  }
  else
  {
    const auto step = 1u << ( var_index - 6u );
    auto j = 0u;
    auto res = then_.construct();

    std::transform( then_.begin(), then_.end(), else_.begin(), res.begin(),
      [&]( auto a, auto b ) {
        return ( j++ % ( 2 * step ) ) < step ? b : a;
      }
    );

    return res;
  }
}

/*! \brief Checks whether two truth tables are equal

  \param first First truth table
  \param second Second truth table
*/
template<typename TT>
inline bool equal( const TT& first, const TT& second )
{
  if ( first.num_vars() != second.num_vars() )
  {
    return false;
  }

  return binary_predicate( first, second, std::equal_to<>() );
}

/*! \cond PRIVATE */
inline bool equal( const partial_truth_table& first, const partial_truth_table& second )
{
  if ( first.num_bits() != second.num_bits() )
  {
    return false;
  }

  return binary_predicate( first, second, std::equal_to<>() );
}
/*! \endcond */

/*! \brief Checks if first truth table implies a second truth table

  \param first First truth table
  \param second Second truth table
*/
template<typename TT>
inline bool implies( const TT& first, const TT& second )
{
  return is_const0( binary_operation( first, second, []( auto a, auto b ) { return ~( ~a | b ); } ) );
}

/*! \brief Checks whether a truth table is lexicographically smaller than another

  Comparison is initiated from most-significant bit.

  \param first First truth table
  \param second Second truth table
*/
template<typename TT>
inline bool less_than( const TT& first, const TT& second )
{
  return std::lexicographical_compare( first._bits.rbegin(), first._bits.rend(),
                                       second._bits.rbegin(), second._bits.rend() );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline bool less_than( const static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second )
{
  return first._bits < second._bits;
}
/*! \endcond */

/*! \brief Checks whether truth table is contant 0

  \param tt Truth table
*/
template<typename TT>
inline bool is_const0( const TT& tt )
{
  return std::all_of( std::begin( tt._bits ), std::end( tt._bits ), []( uint64_t word ) { return word == 0; } );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline bool is_const0( const static_truth_table<NumVars, true>& tt )
{
  return tt._bits == 0;
}
/*! \endcond */

/*! \brief Checks whether truth table depends on given variable index

  \param tt Truth table
  \param var_index Variable index
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
bool has_var( const TT& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );

  if ( tt.num_vars() <= 6 || var_index < 6 )
  {
    return std::any_of( std::begin( tt._bits ), std::end( tt._bits ),
                        [var_index]( uint64_t word ) { return ( ( word >> ( uint64_t( 1 ) << var_index ) ) & detail::projections_neg[var_index] ) !=
                                                              ( word & detail::projections_neg[var_index] ); } );
  }

  const auto step = 1 << ( var_index - 6 );
  for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); i += 2 * step )
  {
    for ( auto j = 0; j < step; ++j )
    {
      if ( tt._bits[i + j] != tt._bits[i + j + step] )
      {
        return true;
      }
    }
  }
  return false;
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
bool has_var( const static_truth_table<NumVars, true>& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );

  return ( ( tt._bits >> ( 1 << var_index ) ) & detail::projections_neg[var_index] ) !=
         ( tt._bits & detail::projections_neg[var_index] );
}
/*! \endcond */

/*! \brief Computes the next lexicographically larger truth table

  This methods updates `tt` to become the next lexicographically
  larger truth table. If `tt` is already the largest truth table, the
  updated truth table will contain all zeros.

  \param tt Truth table
*/
template<typename TT>
void next_inplace( TT& tt )
{
  if ( tt.num_vars() <= 6u )
  {
    tt._bits[0]++;
    tt.mask_bits();
  }
  else
  {
    for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); ++i )
    {
      /* If incrementing the word does not lead to an overflow, we're done*/
      if ( ++tt._bits[i] != 0 )
      {
        break;
      }
    }
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline void next_inplace( static_truth_table<NumVars, true>& tt )
{
  tt._bits++;
  tt.mask_bits();
}
/*! \endcond */

/*! \cond PRIVATE */
inline void next_inplace( partial_truth_table& tt )
{
  for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); ++i )
  {
    /* If incrementing the word does not lead to an overflow, we're done*/
    if ( ++tt._bits[i] != 0u )
    {
      break;
    }
  }
  tt.mask_bits();
}
/*! \endcond */

/*! \brief Returns the next lexicographically larger truth table

  Out-of-place variant for `next_inplace`.

  \param tt Truth table
*/
template<typename TT>
inline TT next( const TT& tt )
{
  auto copy = tt;
  next_inplace( copy );
  return copy;
}

/*! \brief Computes co-factor with respect to 0

  \param tt Truth table
  \param var_index Variable index
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void cofactor0_inplace( TT& tt, uint8_t var_index )
{
  if ( tt.num_vars() <= 6 || var_index < 6 )
  {
    std::transform( std::begin( tt._bits ), std::end( tt._bits ),
                    std::begin( tt._bits ),
                    [var_index]( uint64_t word ) { return ( ( word & detail::projections_neg[var_index] ) << ( uint64_t( 1 ) << var_index ) ) |
                                                          ( word & detail::projections_neg[var_index] ); } );
  }
  else
  {
    const auto step = 1 << ( var_index - 6 );
    for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); i += 2 * step )
    {
      for ( auto j = 0; j < step; ++j )
      {
        tt._bits[i + j + step] = tt._bits[i + j];
      }
    }
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void cofactor0_inplace( static_truth_table<NumVars, true>& tt, uint8_t var_index )
{
  tt._bits = ( ( tt._bits & detail::projections_neg[var_index] ) << ( 1 << var_index ) ) |
             ( tt._bits & detail::projections_neg[var_index] );
}
/*! \endcond */

/*! \brief Returns co-factor with respect to 0

 \param tt Truth table
 \param var_index Variable index
*/
template<typename TT>
TT cofactor0( const TT& tt, uint8_t var_index )
{
  auto copy = tt;
  cofactor0_inplace( copy, var_index );
  return copy;
}

/*! \brief Computes co-factor with respect to 1

  \param tt Truth table
  \param var_index Variable index
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void cofactor1_inplace( TT& tt, uint8_t var_index )
{
  if ( tt.num_vars() <= 6 || var_index < 6 )
  {
    std::transform( std::begin( tt._bits ), std::end( tt._bits ),
                    std::begin( tt._bits ),
                    [var_index]( uint64_t word ) { return ( word & detail::projections[var_index] ) |
                                                          ( ( word & detail::projections[var_index] ) >> ( uint64_t( 1 ) << var_index ) ); } );
  }
  else
  {
    const auto step = 1 << ( var_index - 6 );
    for ( auto i = 0u; i < static_cast<uint32_t>( tt.num_blocks() ); i += 2 * step )
    {
      for ( auto j = 0; j < step; ++j )
      {
        tt._bits[i + j] = tt._bits[i + j + step];
      }
    }
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void cofactor1_inplace( static_truth_table<NumVars, true>& tt, uint8_t var_index )
{
  tt._bits = ( tt._bits & detail::projections[var_index] ) | ( ( tt._bits & detail::projections[var_index] ) >> ( 1 << var_index ) );
}
/*! \endcond */

/*! \brief Returns co-factor with respect to 1

 \param tt Truth table
 \param var_index Variable index
*/
template<typename TT>
TT cofactor1( const TT& tt, uint8_t var_index )
{
  auto copy = tt;
  cofactor1_inplace( copy, var_index );
  return copy;
}

/*! \brief Swaps two adjacent variables in a truth table

  The function swaps variable `var_index` with `var_index + 1`.  The
  function will change `tt` in-place.  If `tt` should not be changed,
  one can use `swap_adjacent` instead.

  \param tt Truth table
  \param var_index A variable
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void swap_adjacent_inplace( TT& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() - 1 );

  /* permute within each word */
  if ( var_index < 5 )
  {
    const auto shift = uint64_t( 1 ) << var_index;
    std::transform( std::begin( tt._bits ), std::end( tt._bits ), std::begin( tt._bits ),
                    [shift, var_index]( uint64_t word ) {
                      return ( word & detail::permutation_masks[var_index][0] ) |
                             ( ( word & detail::permutation_masks[var_index][1] ) << shift ) |
                             ( ( word & detail::permutation_masks[var_index][2] ) >> shift );
                    } );
  }
  /* permute (half) parts of words */
  else if ( var_index == 5 )
  {
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      const auto tmp = *it;
      auto it2 = it + 1;
      *it = ( tmp & 0xffffffff ) | ( *it2 << 0x20 );
      *it2 = ( *it2 & UINT64_C( 0xffffffff00000000 ) ) | ( tmp >> 0x20 );
      it += 2;
    }
  }
  /* permute comlete words */
  else
  {
    const auto step = 1 << ( var_index - 6 );
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      for ( auto i = decltype( step ){0}; i < step; ++i )
      {
        std::swap( *( it + i + step ), *( it + i + 2 * step ) );
      }
      it += 4 * step;
    }
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void swap_adjacent_inplace( static_truth_table<NumVars, true>& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );

  const auto shift = uint64_t( 1 ) << var_index;

  tt._bits = ( tt._bits & detail::permutation_masks[var_index][0] ) |
             ( ( tt._bits & detail::permutation_masks[var_index][1] ) << shift ) |
             ( ( tt._bits & detail::permutation_masks[var_index][2] ) >> shift );
}
/*! \endcond */

/*! \brief Swaps two adjacent variables in a truth table

  The function swaps variable `var_index` with `var_index + 1`.  The
  function will return a new truth table with the result.

  \param tt Truth table
  \param var_index A variable
*/
template<typename TT>
inline TT swap_adjacent( const TT& tt, uint8_t var_index )
{
  auto copy = tt;
  swap_adjacent_inplace( copy, var_index );
  return copy;
}

/*! \brief Swaps two variables in a truth table

  The function swaps variable `var_index1` with `var_index2`.  The
  function will change `tt` in-place.  If `tt` should not be changed,
  one can use `swap` instead.

  \param tt Truth table
  \param var_index1 First variable
  \param var_index2 Second variable
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void swap_inplace( TT& tt, uint8_t var_index1, uint8_t var_index2 )
{
  if ( var_index1 == var_index2 )
  {
    return;
  }

  if ( var_index1 > var_index2 )
  {
    std::swap( var_index1, var_index2 );
  }

  if ( tt.num_vars() <= 6 )
  {
    const auto& pmask = detail::ppermutation_masks[var_index1][var_index2];
    const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
    tt._bits[0] = ( tt._bits[0] & pmask[0] ) | ( ( tt._bits[0] & pmask[1] ) << shift ) | ( ( tt._bits[0] & pmask[2] ) >> shift );
  }
  else if ( var_index2 <= 5 )
  {
    const auto& pmask = detail::ppermutation_masks[var_index1][var_index2];
    const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
    std::transform( std::begin( tt._bits ), std::end( tt._bits ), std::begin( tt._bits ),
                    [shift, &pmask]( uint64_t word ) {
                      return ( word & pmask[0] ) | ( ( word & pmask[1] ) << shift ) | ( ( word & pmask[2] ) >> shift );
                    } );
  }
  else if ( var_index1 <= 5 ) /* in this case, var_index2 > 5 */
  {
    const auto step = 1 << ( var_index2 - 6 );
    const auto shift = 1 << var_index1;
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      for ( auto i = decltype( step ){0}; i < step; ++i )
      {
        const auto low_to_high = ( *( it + i ) & detail::projections[var_index1] ) >> shift;
        const auto high_to_low = ( *( it + i + step ) << shift ) & detail::projections[var_index1];
        *( it + i ) = ( *( it + i ) & ~detail::projections[var_index1] ) | high_to_low;
        *( it + i + step ) = ( *( it + i + step ) & detail::projections[var_index1] ) | low_to_high;
      }
      it += 2 * step;
    }
  }
  else
  {
    const auto step1 = 1 << ( var_index1 - 6 );
    const auto step2 = 1 << ( var_index2 - 6 );
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      for ( auto i = 0; i < step2; i += 2 * step1 )
      {
        for ( auto j = 0; j < step1; ++j )
        {
          std::swap( *( it + i + j + step1 ), *( it + i + j + step2 ) );
        }
      }
      it += 2 * step2;
    }
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline void swap_inplace( static_truth_table<NumVars, true>& tt, uint8_t var_index1, uint8_t var_index2 )
{
  if ( var_index1 == var_index2 )
  {
    return;
  }

  if ( var_index1 > var_index2 )
  {
    std::swap( var_index1, var_index2 );
  }

  const auto& pmask = detail::ppermutation_masks[var_index1][var_index2];
  const auto shift = ( 1 << var_index2 ) - ( 1 << var_index1 );
  tt._bits = ( tt._bits & pmask[0] ) | ( ( tt._bits & pmask[1] ) << shift ) | ( ( tt._bits & pmask[2] ) >> shift );
}
/* \endcond */

/*! \brief Swaps two adjacent variables in a truth table

  The function swaps variable `var_index1` with `var_index2`.  The
  function will return a new truth table with the result.

  \param tt Truth table
  \param var_index1 First variable
  \param var_index2 Second variable
*/
template<typename TT>
inline TT swap( const TT& tt, uint8_t var_index1, uint8_t var_index2 )
{
  auto copy = tt;
  swap_inplace( copy, var_index1, var_index2 );
  return copy;
}

/*! \brief Flips a variable in a truth table

  The function flips variable `var_index` in `tt`.  The function will
  change `tt` in-place.  If `tt` should not be changed, one can use
  `flip` instead.

  \param tt Truth table
  \param var_index A variable
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void flip_inplace( TT& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );

  if ( tt.num_blocks() == 1 )
  {
    const auto shift = 1 << var_index;
    tt._bits[0] = ( ( tt._bits[0] << shift ) & detail::projections[var_index] ) | ( ( tt._bits[0] & detail::projections[var_index] ) >> shift );
  }
  else if ( var_index < 6 )
  {
    const auto shift = 1 << var_index;
    std::transform( std::begin( tt._bits ), std::end( tt._bits ), std::begin( tt._bits ),
                    [var_index, shift]( uint64_t word ) {
                      return ( ( word << shift ) & detail::projections[var_index] ) | ( ( word & detail::projections[var_index] ) >> shift );
                    } );
  }
  else
  {
    const auto step = 1 << ( var_index - 6 );
    auto it = std::begin( tt._bits );
    while ( it != std::end( tt._bits ) )
    {
      for ( auto i = decltype( step ){0}; i < step; ++i )
      {
        std::swap( *( it + i ), *( it + i + step ) );
      }
      it += 2 * step;
    }
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline void flip_inplace( static_truth_table<NumVars, true>& tt, uint8_t var_index )
{
  assert( var_index < tt.num_vars() );

  const auto shift = 1 << var_index;
  tt._bits = ( ( tt._bits << shift ) & detail::projections[var_index] ) | ( ( tt._bits & detail::projections[var_index] ) >> shift );
}
/* \endcond */

/*! \brief Flips a variable in a truth table

  The function flips variable `var_index` in `tt`.  The function will
  not change `tt` and return the result as a copy.

  \param tt Truth table
  \param var_index A variable
*/
template<typename TT>
inline TT flip( const TT& tt, uint8_t var_index )
{
  auto copy = tt;
  flip_inplace( copy, var_index );
  return copy;
}

/*! \brief Reorders truth table to have minimum base

  This function will reorder variables, such that there are no
  "holes".  For example, the function \f$ x_0 \land x_2 \f$ will be
  changed to \f$ x_0 \land x_1 \f$ by swapping \f$ x_1 \f$ with \f$
  x_2 \f$.  That is all variables that are not in the functional
  support will be moved to the back.  Note that the size of the truth
  table is not changed, because for `static_truth_table` one cannot
  compute it at compile-time.

  The function changes the truth table and returns a vector with all
  variable indexes that were in the functional support of the original
  function.

  \param tt Truth table
 */
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
std::vector<uint8_t> min_base_inplace( TT& tt )
{
  std::vector<uint8_t> support;

  auto k = 0u;
  for ( auto i = 0u; i < tt.num_vars(); ++i )
  {
    if ( !has_var( tt, i ) )
    {
      continue;
    }
    if ( k < i )
    {
      swap_inplace( tt, k, i );
    }
    support.push_back( i );
    ++k;
  }

  return support;
}

/*! \brief Expands truth table from minimum base to original based on support

  This is the inverse operation to `min_base_inplace`, where the
  support is used to swap variables back to their original positions.

  \param tt Truth table
  \param support Original indexes of support variables
*/
template<typename TT>
void expand_inplace( TT& tt, const std::vector<uint8_t>& support )
{
  for ( int i = static_cast<int>( support.size() ) - 1; i >= 0; --i )
  {
    assert( i <= support[i] );
    swap_inplace( tt, i, support[i] );
  }
}

/*! \brief Extends smaller truth table to larger one

  The most significant variables will not be in the functional support of the
  resulting truth table, but the method is helpful to align a truth table when
  being used with another one.

  \param tt Larger truth table to create
  \param from Smaller truth table to copy from
*/
template<typename TT, typename TTFrom, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void extend_to_inplace( TT& tt, const TTFrom& from )
{
  assert( tt.num_vars() >= from.num_vars() );

  if ( from.num_vars() < 6 )
  {
    auto mask = *from.begin();

    for ( auto i = from.num_vars(); i < std::min<uint8_t>( 6, tt.num_vars() ); ++i )
    {
      mask |= ( mask << ( 1 << i ) );
    }

    std::fill( tt.begin(), tt.end(), mask );
  }
  else
  {
    auto it = tt.begin();
    while ( it != tt.end() )
    {
      it = std::copy( from.cbegin(), from.cend(), it );
    }
  }
}

/*! \brief Extends smaller truth table to larger static one

  This is an out-of-place version of `extend_to_inplace` that has the truth
  table as a return value.  It only works for creating static truth tables.  The
  template parameter `NumVars` must be equal or larger to the number of
  variables in `from`.

  \param from Smaller truth table to copy from
*/
template<uint32_t NumVars, typename TTFrom>
inline static_truth_table<NumVars> extend_to( const TTFrom& from )
{
  static_truth_table<NumVars> tt;
  extend_to_inplace( tt, from );
  return tt;
}

/*! \brief Extends smaller truth table to larger dynamic one

  This is an out-of-place version of `extend_to_inplace` that has the truth
  table as a return value.  It only works for creating dynamic truth tables.
  The parameter `num_vars` must be equal or larger to the number of variables in
  `from`.

  \param from Smaller truth table to copy from
*/
template<typename TTFrom>
inline dynamic_truth_table extend_to( const TTFrom& from, unsigned num_vars )
{
  auto tt = create<dynamic_truth_table>( num_vars );
  extend_to_inplace( tt, from );
  return tt;
}

/*! \brief Shrinks larger truth table to smaller one

  The function expects that the most significant bits, which are cut off, are
  not in the functional support of the original function.  Only then it is 
  ensured that the resulting function is equivalent. 

  \param tt Smaller truth table to create
  \param from Larger truth table to copy from
*/
template<typename TT, typename TTFrom, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
void shrink_to_inplace( TT& tt, const TTFrom& from )
{
  assert( tt.num_vars() <= from.num_vars() );

  std::copy( from.begin(), from.begin() + tt.num_blocks(), tt.begin() );

  if ( tt.num_vars() < 6 )
  {
    tt.mask_bits();
  }
}

/*! \brief Shrinks larger truth table to smaller static one

  This is an out-of-place version of `shrink_to` that has the truth table as a
  return value.  It only works for creating static truth tables.  The template
  parameter `NumVars` must be equal or smaller to the number of variables in
  `from`.

  \param from Smaller truth table to copy from
*/
template<uint32_t NumVars, typename TTFrom>
inline static_truth_table<NumVars> shrink_to( const TTFrom& from )
{
  static_truth_table<NumVars> tt;
  shrink_to_inplace( tt, from );
  return tt;
}

/*! \brief Shrinks larger truth table to smaller dynamic one

  This is an out-of-place version of `shrink_to` that has the truth table as a
  return value.  It only works for creating dynamic tables.  The parameter
  `num_vars` must be equal or smaller to the number of variables in `from`.

  \param from Smaller truth table to copy from
*/
template<typename TTFrom>
inline dynamic_truth_table shrink_to( const TTFrom& from, unsigned num_vars )
{
  auto tt = create<dynamic_truth_table>( num_vars );
  shrink_to_inplace( tt, from );
  return tt;
}

/*! \brief Left-shift truth table

  Drops overflowing most-significant bits and fills up least-significant bits
  with zeroes.

  \param tt Truth table
  \param shift Number of bits to shift
*/
template<typename TT>
void shift_left_inplace( TT& tt, uint64_t shift )
{
  /* small truth table */
  if ( tt.num_vars() <= 6 )
  {
    tt._bits[0] <<= shift;
    tt.mask_bits();
    return;
  }

  /* large shift */
  if ( shift >= tt.num_bits() )
  {
    clear( tt );
    return;
  }

  if ( shift > 0 )
  {
    const auto last = tt.num_blocks() - 1u;
    const auto div = shift / 64u;
    const auto rem = shift % 64u;

    if ( rem != 0 )
    {
      const auto rshift = 64u - rem;
      for ( auto i = last - div; i > 0; --i )
      {
        tt._bits[i + div] = ( tt._bits[i] << rem ) | ( tt._bits[i - 1] >> rshift );
      }
      tt._bits[div] = tt._bits[0] << rem;
    }
    else
    {
      for ( auto i = last - div; i > 0; --i )
      {
        tt._bits[i + div] = tt._bits[i];
      }
      tt._bits[div] = tt._bits[0];
    }

    std::fill_n( std::begin( tt._bits ), div, 0u );
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline void shift_left_inplace( static_truth_table<NumVars, true>& tt, uint64_t shift )
{
  tt._bits <<= shift;
  tt.mask_bits();
}
/*! \endcond */

/*! \cond PRIVATE */
inline void shift_left_inplace( partial_truth_table& tt, uint64_t shift )
{
  if ( shift >= tt.num_bits() )
  {
    clear( tt );
    return;
  }

  if ( shift > 0u )
  {
    const auto last = tt.num_blocks() - 1u;
    const auto div = shift / 64u;
    const auto rem = shift % 64u;

    if ( rem != 0u )
    {
      const auto rshift = 64u - rem;
      for ( auto i = last - div; i > 0; --i )
      {
        tt._bits[i + div] = ( tt._bits[i] << rem ) | ( tt._bits[i - 1] >> rshift );
      }
      tt._bits[div] = tt._bits[0] << rem;
    }
    else
    {
      for ( auto i = last - div; i > 0; --i )
      {
        tt._bits[i + div] = tt._bits[i];
      }
      tt._bits[div] = tt._bits[0];
    }

    std::fill_n( std::begin( tt._bits ), div, 0u );
    tt.mask_bits();
  }
}
/*! \endcond */

/*! \brief Left-shift truth table

  Out-of-place variant of `shift_left_inplace`.

  \param tt Truth table
  \param shift Number of bits to shift
*/
template<typename TT>
inline TT shift_left( const TT& tt, uint64_t shift )
{
  auto copy = tt;
  shift_left_inplace( copy, shift );
  return copy;
}

/*! \brief Right-shift truth table

  Drops overflowing least-significant bits and fills up most-significant bits
  with zeroes.

  \param tt Truth table
  \param shift Number of bits to shift
*/
template<typename TT>
void shift_right_inplace( TT& tt, uint64_t shift )
{
  /* small truth table */
  if ( tt.num_vars() <= 6 )
  {
    tt._bits[0] >>= shift;
    tt.mask_bits();
    return;
  }

  /* large shift */
  if ( shift >= tt.num_bits() )
  {
    clear( tt );
    return;
  }

  if ( shift > 0 )
  {
    const auto last = tt.num_blocks() - 1u;
    const auto div = shift / 64u;
    const auto rem = shift % 64u;

    if ( rem != 0 )
    {
      const auto rshift = 64u - rem;
      for ( auto i = div; i < last; ++i )
      {
        tt._bits[i - div] = ( tt._bits[i] >> rem ) | ( tt._bits[i + 1] << rshift );
      }
      tt._bits[last - div] = tt._bits[last] >> rem;
    }
    else
    {
      for ( auto i = div; i <= last; ++i )
      {
        tt._bits[i - div] = tt._bits[i];
      }
    }

    std::fill_n( std::begin( tt._bits ) + ( tt.num_blocks() - div ), div, 0u );
  }
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline void shift_right_inplace( static_truth_table<NumVars, true>& tt, uint64_t shift )
{
  tt._bits >>= shift;
}
/*! \endcond */

/*! \cond PRIVATE */
inline void shift_right_inplace( partial_truth_table& tt, uint64_t shift )
{
  if ( shift >= tt.num_bits() )
  {
    clear( tt );
    return;
  }

  if ( shift > 0u )
  {
    tt.mask_bits();

    const auto last = tt.num_blocks() - 1u;
    const auto div = shift / 64u;
    const auto rem = shift % 64u;

    if ( rem != 0u )
    {
      const auto rshift = 64u - rem;
      for ( auto i = div; i < last; ++i )
      {
        tt._bits[i - div] = ( tt._bits[i] >> rem ) | ( tt._bits[i + 1] << rshift );
      }
      tt._bits[last - div] = tt._bits[last] >> rem;
    }
    else
    {
      for ( auto i = div; i <= last; ++i )
      {
        tt._bits[i - div] = tt._bits[i];
      }
    }

    std::fill_n( std::begin( tt._bits ) + ( tt.num_blocks() - div ), div, 0u );
  }
}
/*! \endcond */

/*! \brief Right-shift truth table

  Out-of-place variant of `shift_right_inplace`.

  \param tt Truth table
  \param shift Number of bits to shift
*/
template<typename TT>
inline TT shift_right( const TT& tt, uint64_t shift )
{
  auto copy = tt;
  shift_right_inplace( copy, shift );
  return copy;
}

/*! \brief Composes a truth table.

  Given a function `f`, and a set of truth tables as arguments, computes the
  composed truth table.  For example, if `f(x1, x2) = 1001` and
  `vars = {x1 = 1001, x2= 1010}`, the function returns 1000. This function can
  be regarded as a general operator with arity `vars.size()` where the behavior
  of the operator is given by f.

  \param f The outer function
  \param vars The ordered set of input variables
  \return The composed truth table with vars.size() variables
*/
template<class TTf, class TTv, typename = std::enable_if_t<is_complete_truth_table<TTf>::value>>
inline auto compose_truth_table( const TTf& f, const std::vector<TTv>& vars )
{
  assert( vars.size() == static_cast<std::size_t>( f.num_vars() ) );
  auto composed = vars[0].construct();

  for ( uint64_t i = 0u; i < composed.num_bits(); ++i )
  {
    uint64_t index = 0u;
    for ( uint64_t j = 0u; j < vars.size(); ++j )
    {
      index += get_bit( vars[j], i ) << j;
    }
    if ( get_bit( f, index ) )
    {
      set_bit( composed, i );
    }
    else
    {
      clear_bit( composed, i );
    }
  }

  return composed;
}

/*! \brief Shifts a small truth table with respect to a mask

  This function only works for truth tables with up to 6 inputs.  The function
  rearranges the variables according to a mask.  For example, assume the 3-input
  truth table \f$ x_0 \land x_1 \f$, which is not defined on \f$ x_2 \f$.
  Applying this funtion with a mask `0b101` yields the function
  \f$ x_0 \land x_2 \f$, and the mask `0b110` yields the function
  \f$ x_1 \land x_2 \f$.  The bits in the mask provide the new positions.  It
  is important that the positions in the mask do not exceed the truth table
  size, since all operations are performed in-place and cannot change the
  number of variables of the truth table.

  \param tt Truth table
  \param mask Shift mask
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
inline void shift_with_mask_inplace( TT& f, uint8_t mask )
{
  assert( f.num_vars() <= 6 );

  *f.begin() = detail::compute_shift( *f.begin(), mask | ( 1 << f.num_vars() ) );
}

/*! \brief Shifts a small truth table with respect to a mask

  Out-of-place variant of `shift_with_mask_inplace`.

  \param tt Truth table
  \param mask Shift mask
*/
template<class TT>
inline TT shift_with_mask( const TT& f, uint8_t mask )
{
  auto copy = f;
  shift_with_mask_inplace( copy, mask );
  return copy;
}

} // namespace kitty
