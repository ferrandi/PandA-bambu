/* kitty: C++ truth table library
 * Copyright (C) 2017-2019  EPFL
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
  \file properties.hpp
  \brief Implements property checks for Boolean function
  \author Mathias Soeken
*/

#pragma once

#include <cinttypes>
#include <utility>
#include <vector>

#include "bit_operations.hpp"
#include "operations.hpp"
#include "operators.hpp"

namespace kitty
{

/*! \brief Returns the Chow parameter of a function
  The Chow parameters is a set of values \f$N(f), \Sigma(f)\f$, where \f$N(f)\f$
  is the size of the ON-set, and \f$\Sigma(f)\f$ is the sum of all input
  assignments in the ON-set.  For example for \f$f = x_1 \lor x_2\f$ the
  function returns \f$(3, (2,2))\f$.
  \param tt Truth table
*/
template<typename TT>
std::pair<uint32_t, std::vector<uint32_t>> chow_parameters( const TT& tt )
{
  assert( tt.num_vars() <= 32 );

  const auto n = tt.num_vars();
  const auto nf = count_ones( tt );

  std::vector<uint32_t> sf( n, 0u );
  for_each_one_bit( tt, [&sf]( auto minterm ) {
    for ( auto i = 0u; minterm; ++i )
    {
      if ( minterm & 1 )
      {
        ++sf[i];
      }
      minterm >>= 1;
    }
  } );

  return {nf, sf};
}

/*! \brief Checks whether a function is canalizing
  \param tt Truth table
*/
template<typename TT>
bool is_canalizing( const TT& tt )
{
  uint32_t f1or{}, f0or{};
  uint32_t f1and, f0and;

  uint32_t max = static_cast<uint32_t>( ( uint64_t( 1 ) << tt.num_vars() ) - 1 );
  f1and = f0and = max;

  for ( uint32_t i = 0u; i < tt.num_bits(); ++i )
  {
    if ( get_bit( tt, i ) == 0 )
    {
      f0and &= i;
      f0or |= i;
    }
    else
    {
      f1and &= i;
      f1or |= i;
    }

    if ( f0and == 0 && f1and == 0 && f0or == max && f1or == max )
    {
      return false;
    }
  }

  return true;
}

/*! \brief Checks whether a function is Horn
  A function is Horn, if it can be represented using Horn clauses.
  \param tt Truth table
*/
template<typename TT>
bool is_horn( const TT& tt )
{
  for ( uint32_t i = 1u; i < tt.num_bits(); ++i )
  {
    for ( uint32_t j = 0u; j < i; ++j )
    {
      if ( get_bit( tt, j ) && get_bit( tt, i ) && !get_bit( tt, i & j ) )
      {
        return false;
      }
    }
  }

  return true;
}

/*! \brief Checks whether a function is Krom
  A function is Krom, if it can be represented using Krom clauses.
  \param tt Truth table
*/
template<typename TT>
bool is_krom( const TT& tt )
{
  for ( uint32_t i = 2u; i < tt.num_bits(); ++i )
  {
    for ( uint32_t j = 1u; j < i; ++j )
    {
      for ( uint32_t k = 0u; k < j; ++k )
      {
        const auto maj = ( i & j ) | ( i & k ) | ( j & k );
        if ( get_bit( tt, k ) && get_bit( tt, j ) && get_bit( tt, i ) && !get_bit( tt, maj ) )
        {
          return false;
        }
      }
    }
  }

  return true;
}

/*! \brief Checks whether a function is symmetric in a pair of variables
  A function is symmetric in two variables, if it is invariant to swapping them.
  \param tt Truth table
  \param var_index1 Index of first variable
  \param var_index2 Index of second variable
*/
template<typename TT>
bool is_symmetric_in( const TT& tt, uint8_t var_index1, uint8_t var_index2 )
{
  return tt == swap( tt, var_index1, var_index2 );
}

/*! \brief Checks whether a function is monotone
  A function is monotone if f(x) ≤ f(y) whenever x ⊆ y
  \param tt Truth table
*/
template<typename TT>
bool is_monotone( const TT& tt )
{
  auto numvars = tt.num_vars();

  for ( auto i = 0; i < numvars; i++ )
  {
    auto const tt1 = cofactor0( tt, i );
    auto const tt2 = cofactor1( tt, i );
    for ( auto bit = 0; bit < ( 2 << ( numvars - 1 ) ); bit++ )
    {
      if ( get_bit( tt1, bit ) <= get_bit( tt2, bit ) )
      {
        continue;
      }
      else
      {
        return false;
      }
    }
  }
  return true;
}

/*! \brief Checks whether a function is selfdual
  A function is selfdual if !f(x, y, ..., z) = f(!x, !y, ..., !z)
  \param tt Truth table
*/
template<typename TT>
bool is_selfdual( const TT& tt )
{
  auto numvars = tt.num_vars();
  auto tt1 = tt;
  auto tt2 = ~tt1;
  for ( auto i = 0; i < numvars; i++ )
  {
    tt1 = flip( tt1, i );
  }

  return tt2 == tt1;
}

/*! \brief Checks if a function is normal
  A function is normal iff f(0, ..., 0) = 0.
  \param tt Truth table
*/
template<typename TT>
bool is_normal( const TT& tt )
{
  return !get_bit( tt, 0u );
}

/*! \brief Checks if a function is trivial
  A function is trival if it is equal to (or the complement of) a
  variable or constant zero.
  \param tt Truth table
*/
template<typename TT>
bool is_trivial( const TT& tt )
{
  /* compare to constants */
  if ( is_const0( tt ) || is_const0( ~tt ) )
    return true;

  /* compare to variables */
  TT tt_check = tt;
  for ( auto i = 0; i < tt.num_vars(); ++i )
  {
    create_nth_var( tt_check, i );
    if ( tt == tt_check || tt == ~tt_check )
      return true;
  }

  return false;
}

/*! \brief Generate runlength encoding of a function
  This function iterates through the bits of a function and calls a function
  for each runlength and value.  For example, if this function is called for
  the AND function 1000, it will call `fn` first with arguments `false` and `3`,
  and then a second time with arguments `true`, and `1`.
  \param tt Truth table
  \param fn Function of signature `void(bool, uint32_t)`
*/
template<typename TT, typename Fn>
void foreach_runlength( const TT& tt, Fn&& fn )
{
  bool current = get_bit( tt, 0 );
  uint32_t length{1u};

  for ( auto i = 1ull; i < tt.num_bits(); ++i )
  {
    if ( get_bit( tt, i ) != current )
    {
      fn( current, length );
      current = !current;
      length = 1u;
    }
    else
    {
      ++length;
    }
  }

  fn( current, length );
}

/*! \brief Returns the runlength encoding pattern of a function
  This function does only count the lengths, e.g., for 1000 it will return
  `{3, 1}`, and so it does for the NAND function 0111.
  \param tt Truth table
*/
template<typename TT>
std::vector<uint32_t> runlength_pattern( const TT& tt )
{
  std::vector<uint32_t> pattern;
  foreach_runlength( tt, [&]( bool, uint32_t length ) {
    pattern.push_back( length );
  } );
  return pattern;
}

} // namespace kitty
