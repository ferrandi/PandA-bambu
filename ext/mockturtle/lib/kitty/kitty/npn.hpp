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
  \file npn.hpp
  \brief Implements NPN canonization algorithms

  \author Mathias Soeken
*/

#pragma once

#include <numeric>

#include "detail/constants.hpp"
#include "operators.hpp"
#include "traits.hpp"

namespace kitty
{

/*! \cond PRIVATE */

namespace detail
{
template<typename TT>
void exact_npn_canonization_null_callback( const TT& tt )
{
  (void)tt;
}
} /* namespace detail */
/*! \endcond */

/*! \brief Exact P canonization

  Given a truth table, this function finds the lexicographically smallest truth
  table in its P class, called P representative. Two functions are in the
  same P class, if one can obtain one from the other by input permutation.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole P class.

  The function returns a NPN configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the P representative
  - input negations and output negation, which is 0 in this case
  - input permutation to apply

  \param tt The truth table
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return NPN configuration
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, uint32_t, std::vector<uint8_t>> exact_p_canonization( const TT& tt, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    return std::make_tuple( tt, 0u, std::vector<uint8_t>{} );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    return std::make_tuple( tt, 0u, std::vector<uint8_t>{0} );
  }

  assert( num_vars >= 2 && num_vars <= 7 );

  auto t1 = tt;
  auto tmin = t1;

  fn( t1 );

  const auto& swaps = detail::swaps[num_vars - 2u];

  int best_swap = -1;

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );

    fn( t1 );

    if ( t1 < tmin )
    {
      best_swap = static_cast<int>( i );
      tmin = t1;
    }
  }

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  for ( auto i = 0; i <= best_swap; ++i )
  {
    const auto pos = swaps[i];
    std::swap( perm[pos], perm[pos + 1] );
  }

  return std::make_tuple( tmin, 0u, perm );
}

/*! \brief Exact NPN canonization

  Given a truth table, this function finds the lexicographically smallest truth
  table in its NPN class, called NPN representative. Two functions are in the
  same NPN class, if one can obtain one from the other by input negation, input
  permutation, and output negation.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole NPN class.

  The function returns a NPN configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the NPN representative
  - input negations and output negation, output negation is stored as bit *n*,
    where *n* is the number of variables in `tt`
  - input permutation to apply

  \param tt The truth table (with at most 6 variables)
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return NPN configuration
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, uint32_t, std::vector<uint8_t>> exact_npn_canonization( const TT& tt, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    const auto bit = get_bit( tt, 0 );
    return std::make_tuple( unary_not_if( tt, bit ), static_cast<uint32_t>( bit ), std::vector<uint8_t>{} );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    const auto bit1 = get_bit( tt, 1 );
    return std::make_tuple( unary_not_if( tt, bit1 ), static_cast<uint32_t>( bit1 << 1 ), std::vector<uint8_t>{0} );
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt, t2 = ~tt;
  auto tmin = std::min( t1, t2 );
  auto invo = tmin == t2;

  fn( t1 );
  fn( t2 );

  const auto& swaps = detail::swaps[num_vars - 2u];
  const auto& flips = detail::flips[num_vars - 2u];

  int best_swap = -1;
  int best_flip = -1;

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );
    swap_adjacent_inplace( t2, pos );

    fn( t1 );
    fn( t2 );

    if ( t1 < tmin || t2 < tmin )
    {
      best_swap = static_cast<int>( i );
      tmin = std::min( t1, t2 );
      invo = tmin == t2;
    }
  }

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    swap_adjacent_inplace( t1, 0 );
    flip_inplace( t1, pos );
    swap_adjacent_inplace( t2, 0 );
    flip_inplace( t2, pos );

    fn( t1 );
    fn( t2 );

    if ( t1 < tmin || t2 < tmin )
    {
      best_swap = -1;
      best_flip = static_cast<int>( j );
      tmin = std::min( t1, t2 );
      invo = tmin == t2;
    }

    for ( std::size_t i = 0; i < swaps.size(); ++i )
    {
      const auto pos = swaps[i];
      swap_adjacent_inplace( t1, pos );
      swap_adjacent_inplace( t2, pos );

      fn( t1 );
      fn( t2 );

      if ( t1 < tmin || t2 < tmin )
      {
        best_swap = static_cast<int>( i );
        best_flip = static_cast<int>( j );
        tmin = std::min( t1, t2 );
        invo = tmin == t2;
      }
    }
  }

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  for ( auto i = 0; i <= best_swap; ++i )
  {
    const auto pos = swaps[i];
    std::swap( perm[pos], perm[pos + 1] );
  }

  uint32_t phase = uint32_t( invo ) << num_vars;
  for ( auto i = 0; i <= best_flip; ++i )
  {
    phase ^= 1 << flips[i];
  }

  return std::make_tuple( tmin, phase, perm );
}

/*! \brief Flip-swap NPN heuristic

  This algorithm will iteratively try to reduce the numeric value of the truth
  table by first inverting each input, then inverting the output, and then
  swapping each pair of inputs.  Every improvement is accepted, the algorithm
  stops, if no more improvement can be achieved.

  The function returns a NPN configuration which contains the
  necessary transformations to obtain the representative.  It is a
  tuple of

  - the NPN representative
  - input negations and output negation, output negation is stored as
    bit *n*, where *n* is the number of variables in `tt`
  - input permutation to apply

  \param tt Truth table
  \return NPN configuration
*/
template<typename TT>
std::tuple<TT, uint32_t, std::vector<uint8_t>> flip_swap_npn_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* initialize permutation and phase */
  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  uint32_t phase{0u};

  auto npn = tt;
  auto improvement = true;

  while ( improvement )
  {
    improvement = false;

    /* input inversion */
    for ( auto i = 0u; i < num_vars; ++i )
    {
      const auto flipped = flip( npn, i );
      if ( flipped < npn )
      {
        npn = flipped;
        phase ^= 1 << perm[i];
        improvement = true;
      }
    }

    /* output inversion */
    const auto flipped = ~npn;
    if ( flipped < npn )
    {
      npn = flipped;
      phase ^= 1 << num_vars;
      improvement = true;
    }

    /* permute inputs */
    for ( auto d = 1u; d < num_vars - 1; ++d )
    {
      for ( auto i = 0u; i < num_vars - d; ++i )
      {
        auto j = i + d;

        const auto permuted = swap( npn, i, j );
        if ( permuted < npn )
        {
          npn = permuted;
          std::swap( perm[i], perm[j] );

          improvement = true;
        }
      }
    }
  }

  return std::make_tuple( npn, phase, perm );
}

/*! \cond PRIVATE */
namespace detail
{

template<typename TT>
void sifting_npn_canonization_loop( TT& npn, uint32_t& phase, std::vector<uint8_t>& perm )
{
  auto improvement = true;
  auto forward = true;

  const auto n = npn.num_vars();

  while ( improvement )
  {
    improvement = false;

    for ( int i = forward ? 0 : n - 2; forward ? i < static_cast<int>( n - 1 ) : i >= 0; forward ? ++i : --i )
    {
      auto local_improvement = false;
      for ( auto k = 1u; k < 8u; ++k )
      {
        if ( k % 4u == 0u )
        {
          const auto next_t = swap( npn, i, i + 1 );
          if ( next_t < npn )
          {
            npn = next_t;
            std::swap( perm[i], perm[i + 1] );
            local_improvement = true;
          }
        }
        else if ( k % 2u == 0u )
        {
          const auto next_t = flip( npn, i + 1 );
          if ( next_t < npn )
          {
            npn = next_t;
            phase ^= 1 << perm[i + 1];
            local_improvement = true;
          }
        }
        else
        {
          const auto next_t = flip( npn, i );
          if ( next_t < npn )
          {
            npn = next_t;
            phase ^= 1 << perm[i];
            local_improvement = true;
          }
        }
      }

      if ( local_improvement )
      {
        improvement = true;
      }
    }

    forward = !forward;
  }
}
} /* namespace detail */
/*! \endcond */

/*! \brief Sifting NPN heuristic

  The algorithm will always consider two adjacent variables and try all possible
  transformations on these two.  It will try once in forward direction and once
  in backward direction.  It will try for the regular function and inverted
  function.

  The function returns a NPN configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the NPN representative
  - input negations and output negation, output negation is stored as bit *n*,
    where *n* is the number of variables in `tt`
  - input permutation to apply

  \param tt Truth table
  \return NPN configuration
*/
template<typename TT>
std::tuple<TT, uint32_t, std::vector<uint8_t>> sifting_npn_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* initialize permutation and phase */
  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );
  uint32_t phase{0u};

  if ( num_vars < 2 )
  {
    return std::make_tuple( tt, phase, perm );
  }

  auto npn = tt;

  detail::sifting_npn_canonization_loop( npn, phase, perm );

  const auto best_perm = perm;
  const auto best_phase = phase;
  const auto best_npn = npn;

  npn = ~tt;
  phase = 1 << num_vars;
  std::iota( perm.begin(), perm.end(), 0u );

  detail::sifting_npn_canonization_loop( npn, phase, perm );

  if ( best_npn < npn )
  {
    perm = best_perm;
    phase = best_phase;
    npn = best_npn;
  }

  return std::make_tuple( npn, phase, perm );
}

/*! \brief Obtain truth table from NPN configuration

  Given an NPN configuration, which contains a representative
  function, input/output negations, and input permutations this
  function computes the original truth table.

  \param config NPN configuration
*/
template<typename TT>
TT create_from_npn_config( const std::tuple<TT, uint32_t, std::vector<uint8_t>>& config )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto& from = std::get<0>( config );
  const auto& phase = std::get<1>( config );
  auto perm = std::get<2>( config );
  const auto num_vars = from.num_vars();

  /* is output complemented? */
  auto res = ( ( phase >> num_vars ) & 1 ) ? ~from : from;

  /* input permutations */
  for ( auto i = 0u; i < num_vars; ++i )
  {
    if ( perm[i] == i )
    {
      continue;
    }

    int k = i;
    while ( perm[k] != i )
    {
      ++k;
    }

    swap_inplace( res, i, k );
    std::swap( perm[i], perm[k] );
  }

  /* input complementations */
  for ( auto i = 0u; i < num_vars; ++i )
  {
    if ( ( phase >> i ) & 1 )
    {
      flip_inplace( res, i );
    }
  }

  return res;
}

} /* namespace kitty */
