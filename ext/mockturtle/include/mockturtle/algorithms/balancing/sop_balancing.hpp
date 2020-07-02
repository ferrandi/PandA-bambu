/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2019  EPFL
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
  \file sop_balancing.hpp
  \brief SOP-based balancing engine for `balancing` algorithm

  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/isop.hpp>
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>

#include "../../traits.hpp"
#include "../../utils/stopwatch.hpp"
#include "../balancing.hpp"

namespace mockturtle
{

template<class Ntk>
struct arrival_time_compare
{
  bool operator()( arrival_time_pair<Ntk> const& p1, arrival_time_pair<Ntk> const& p2 ) const
  {
    return p1.level > p2.level;
  }
};

template<class Ntk>
using arrival_time_queue = std::priority_queue<arrival_time_pair<Ntk>, std::vector<arrival_time_pair<Ntk>>, arrival_time_compare<Ntk>>;

/*! \brief SOP rebalancing function
 *
 * This class can be used together with the generic `balancing` function.  It
 * converts each cut function into an SOP and then performs weight-oriented
 * tree balancing on the AND terms and the outer OR function.
 */
template<class Ntk>
struct sop_rebalancing
{
  void operator()( Ntk& dest, kitty::dynamic_truth_table const& function, std::vector<arrival_time_pair<Ntk>> const& inputs, uint32_t best_level, uint32_t best_cost, rebalancing_function_callback_t<Ntk> const& callback ) const
  {
    auto [and_terms, num_and_gates] = create_function( dest, function, inputs );
    const auto num_gates = num_and_gates + ( and_terms.empty() ? 0u : static_cast<uint32_t>( and_terms.size() ) - 1u );
    const auto cand = balanced_tree( dest, and_terms, false );

    if ( cand.level < best_level || ( cand.level == best_level && num_gates < best_cost ) )
    {
      callback( cand, num_gates );
    }
  }

private:
  std::pair<arrival_time_queue<Ntk>, uint32_t> create_function( Ntk& dest, kitty::dynamic_truth_table const& func, std::vector<arrival_time_pair<Ntk>> const& arrival_times ) const
  {
    const auto sop = create_sop_form( func );

    stopwatch<> t_tree( time_tree_balancing );
    arrival_time_queue<Ntk> and_terms;
    uint32_t num_and_gates{};
    for ( auto const& cube : sop )
    {
      arrival_time_queue<Ntk> product_queue;
      for ( auto i = 0u; i < func.num_vars(); ++i )
      {
        if ( cube.get_mask( i ) )
        {
          const auto [f, l] = arrival_times[i];
          product_queue.push( {cube.get_bit( i ) ? f : dest.create_not( f ), l} );
        }
      }
      if ( !product_queue.empty() )
      {
        num_and_gates += static_cast<uint32_t>( product_queue.size() ) - 1u;
      }
      and_terms.push( balanced_tree( dest, product_queue ) );
    }
    return {and_terms, num_and_gates};
  }

  arrival_time_pair<Ntk> balanced_tree( Ntk& dest, arrival_time_queue<Ntk>& queue, bool _and = true ) const
  {
    if ( queue.empty() )
    {
      return {dest.get_constant( true ), 0u};
    }

    while ( queue.size() > 1u )
    {
      auto [s1, l1] = queue.top();
      queue.pop();
      auto [s2, l2] = queue.top();
      queue.pop();
      const auto s = _and ? dest.create_and( s1, s2 ) : dest.create_or( s1, s2 );
      const auto l = std::max( l1, l2 ) + 1;
      queue.push( {s, l} );
    }
    return queue.top();
  }

  std::vector<kitty::cube> create_sop_form( kitty::dynamic_truth_table const& func ) const
  {
    stopwatch<> t( time_sop );
    if ( auto it = sop_hash_.find( func ); it != sop_hash_.end() )
    {
      sop_cache_hits++;
      return it->second;
    }
    else
    {
      sop_cache_misses++;
      return sop_hash_[func] = kitty::isop( func ); // TODO generalize
    }
  }

private:
  mutable std::unordered_map<kitty::dynamic_truth_table, std::vector<kitty::cube>, kitty::hash<kitty::dynamic_truth_table>> sop_hash_;

public:
  mutable uint32_t sop_cache_hits{};
  mutable uint32_t sop_cache_misses{};

  mutable stopwatch<>::duration time_sop{};
  mutable stopwatch<>::duration time_tree_balancing{};
};

} // namespace mockturtle
