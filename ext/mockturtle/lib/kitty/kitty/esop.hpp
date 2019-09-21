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
  \file esop.hpp
  \brief Implements methods to compute exclusive sum-of-products (ESOP) representations

  \author Mathias Soeken
  \author Winston Haaswijk
*/

#pragma once

#warning "DEPRECATED: the functions in this file are marked as deprecated.  Most recent implementation can be found in https://github.com/hriener/easy/ in the file src/esop/constructors.hpp"

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "cube.hpp"
#include "hash.hpp"
#include "operations.hpp"
#include "operators.hpp"

namespace kitty
{

/*! \cond PRIVATE */
namespace detail
{

enum class pkrm_decomposition
{
  positive_davio,
  negative_davio,
  shannon
};

template<typename TT>
using expansion_cache = std::unordered_map<TT, std::pair<uint32_t, pkrm_decomposition>, hash<TT>>;

template<typename TT>
uint32_t find_pkrm_expansions( const TT& tt, expansion_cache<TT>& cache, uint8_t var_index )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return 0;
  }
  if ( is_const0( ~tt ) )
  {
    return 1;
  }

  /* already computed */
  const auto it = cache.find( tt );
  if ( it != cache.end() )
  {
    return it->second.first;
  }

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  const auto ex0 = find_pkrm_expansions( tt0, cache, var_index + 1 );
  const auto ex1 = find_pkrm_expansions( tt1, cache, var_index + 1 );
  const auto ex2 = find_pkrm_expansions( tt0 ^ tt1, cache, var_index + 1 );

  const auto ex_max = std::max( std::max( ex0, ex1 ), ex2 );

  uint32_t cost{};
  pkrm_decomposition decomp;

  if ( ex_max == ex0 )
  {
    cost = ex1 + ex2;
    decomp = pkrm_decomposition::positive_davio;
  }
  else if ( ex_max == ex1 )
  {
    cost = ex0 + ex2;
    decomp = pkrm_decomposition::negative_davio;
  }
  else
  {
    cost = ex0 + ex1;
    decomp = pkrm_decomposition::shannon;
  }
  cache.insert( {tt, {cost, decomp}} );
  return cost;
}

inline void add_to_cubes( std::unordered_set<cube, hash<cube>>& pkrm, const cube& c, bool distance_one_merging = true )
{
  /* first check whether cube is already contained; if so, delete it */
  const auto it = pkrm.find( c );
  if ( it != pkrm.end() )
  {
    pkrm.erase( it );
    return;
  }

  /* otherwise, check if there is a distance-1 cube; if so, merge it */
  if ( distance_one_merging )
  {
    for ( auto it = pkrm.begin(); it != pkrm.end(); ++it )
    {
      if ( c.distance( *it ) == 1 )
      {
        auto new_cube = c.merge( *it );
        pkrm.erase( it );
        add_to_cubes( pkrm, new_cube );
        return;
      }
    }
  }

  /* otherwise, just add the cube */
  pkrm.insert( c );
}

inline cube with_literal( const cube& c, uint8_t var_index, bool polarity )
{
  auto copy = c;
  copy.add_literal( var_index, polarity );
  return copy;
}

template<typename TT>
void optimum_pkrm_rec( std::unordered_set<cube, hash<cube>>& pkrm, const TT& tt, const expansion_cache<TT>& cache, uint8_t var_index, const cube& c )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return;
  }
  if ( is_const0( ~tt ) )
  {
    add_to_cubes( pkrm, c );
    return;
  }

  const auto& p = cache.at( tt );

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  switch ( p.second )
  {
  case pkrm_decomposition::positive_davio:
    optimum_pkrm_rec( pkrm, tt0, cache, var_index + 1, c );
    optimum_pkrm_rec( pkrm, tt0 ^ tt1, cache, var_index + 1, with_literal( c, var_index, true ) );
    break;
  case pkrm_decomposition::negative_davio:
    optimum_pkrm_rec( pkrm, tt1, cache, var_index + 1, c );
    optimum_pkrm_rec( pkrm, tt0 ^ tt1, cache, var_index + 1, with_literal( c, var_index, false ) );
    break;
  case pkrm_decomposition::shannon:
    optimum_pkrm_rec( pkrm, tt0, cache, var_index + 1, with_literal( c, var_index, false ) );
    optimum_pkrm_rec( pkrm, tt1, cache, var_index + 1, with_literal( c, var_index, true ) );
    break;
  }
}

template<typename TT>
void esop_from_pprm_rec( std::unordered_set<cube, hash<cube>>& cubes, const TT& tt, uint8_t var_index, const cube& c )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return;
  }
  if ( is_const0( ~tt ) )
  {
    /* add to cubes, but do not apply distance-1 merging */
    add_to_cubes( cubes, c, false );
    return;
  }

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  esop_from_pprm_rec( cubes, tt0, var_index + 1, c );
  esop_from_pprm_rec( cubes, tt0 ^ tt1, var_index + 1, with_literal( c, var_index, true ) );
}
} // namespace detail
/*! \endcond */

/*! \brief Computes ESOP representation using optimum PKRM

  This algorithm first computes an ESOP using the algorithm described
  in [R. Drechsler, IEEE Trans. C 48(9), 1999, 987–990].

  The algorithm applies post-optimization to merge distance-1 cubes.

  \param tt Truth table
*/
template<typename TT>
inline std::vector<cube> esop_from_optimum_pkrm( const TT& tt )
{
  std::unordered_set<cube, hash<cube>> cubes;
  detail::expansion_cache<TT> cache;

  detail::find_pkrm_expansions( tt, cache, 0 );
  detail::optimum_pkrm_rec( cubes, tt, cache, 0, cube() );

  return std::vector<cube>( cubes.begin(), cubes.end() );
}

/*! \brief Computes PPRM representation for a function

  This algorithm applies recursively the positive Davio decomposition which
  eventually leads into the PPRM representation of a function.

  \param tt Truth table
*/
template<typename TT>
inline std::vector<cube> esop_from_pprm( const TT& tt )
{
  std::unordered_set<cube, hash<cube>> cubes;
  detail::esop_from_pprm_rec( cubes, tt, 0, cube() );

  return std::vector<cube>( cubes.begin(), cubes.end() );
}

} // namespace kitty
