/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
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
  \file esop_balancing.hpp
  \brief ESOP-based balancing engine for `balancing` algorithm

  \author Heinz Riener
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
#include <kitty/esop.hpp>
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>
#include <kitty/spp.hpp>

#include "../../traits.hpp"
#include "../../utils/stopwatch.hpp"
#include "../balancing.hpp"
#include "../exorcism.hpp"
#include "utils.hpp"

namespace mockturtle
{

template<class Ntk>
struct esop_rebalancing
{
  void operator()( Ntk& dest, kitty::dynamic_truth_table const& function, std::vector<arrival_time_pair<Ntk>> const& inputs, uint32_t best_level, uint32_t best_cost, rebalancing_function_callback_t<Ntk> const& callback ) const
  {
    const auto [and_terms, max_level, num_and_gates] = create_function( dest, function, inputs );

    /* Try with MUX decomposition */
    if ( mux_optimization )
    {
      const auto index = std::distance( inputs.begin(), std::max_element( inputs.begin(), inputs.end(), []( auto const& a1, auto const& a2 ) { return a1.level < a2.level; } ) );

      const auto [and_terms0, max_level0, num_and_gates0] = create_function( dest, kitty::cofactor0( function, static_cast<uint8_t>( index ) ), inputs );
      const auto [and_terms1, max_level1, num_and_gates1] = create_function( dest, kitty::cofactor1( function, static_cast<uint8_t>( index ) ), inputs );

      const auto max_level_mux = std::max( max_level0, max_level1 ) + 1u;

      if ( max_level_mux < max_level && max_level_mux < best_level )
      {
        callback( { dest.create_ite( inputs[index].f,
                                     dest.create_nary_xor( and_terms1 ),
                                     dest.create_nary_xor( and_terms0 ) ),
                    max_level_mux },
                  num_and_gates0 + num_and_gates1 + 1u );
        return;
      }
    }

    if ( max_level < best_level || ( max_level == best_level && num_and_gates < best_cost ) )
    {
      callback( { dest.create_nary_xor( and_terms ), max_level }, num_and_gates );
    }
  }

private:
  std::tuple<std::vector<signal<Ntk>>, uint32_t, uint32_t> create_function( Ntk& dest, kitty::dynamic_truth_table const& func, std::vector<arrival_time_pair<Ntk>> const& arrival_times ) const
  {
    if ( spp_optimization )
    {
      return create_function_from_spp( dest, func, arrival_times );
    }
    else
    {
      return create_function_from_esop( dest, func, arrival_times );
    }
  }

  std::tuple<std::vector<signal<Ntk>>, uint32_t, uint32_t> create_function_from_esop( Ntk& dest, kitty::dynamic_truth_table const& func, std::vector<arrival_time_pair<Ntk>> const& arrival_times ) const
  {
    const auto esop = create_sop_form( func );

    stopwatch<> t_tree( time_tree_balancing );
    std::vector<signal<Ntk>> and_terms;
    uint32_t max_level{};
    uint32_t num_and_gates{};
    for ( auto const& cube : esop )
    {
      arrival_time_queue<Ntk> product_queue;
      for ( auto i = 0u; i < func.num_vars(); ++i )
      {
        if ( cube.get_mask( i ) )
        {
          const auto [f, l] = arrival_times[i];
          product_queue.push( { cube.get_bit( i ) ? f : dest.create_not( f ), l } );
        }
      }
      if ( product_queue.size() )
      {
        num_and_gates += static_cast<uint32_t>( product_queue.size() ) - 1u;
      }
      auto [s, l] = balanced_and_tree( dest, product_queue );
      and_terms.push_back( s );
      max_level = std::max( max_level, l );
    }
    return { and_terms, max_level, num_and_gates };
  }

  std::tuple<std::vector<signal<Ntk>>, uint32_t, uint32_t> create_function_from_spp( Ntk& dest, kitty::dynamic_truth_table const& func, std::vector<arrival_time_pair<Ntk>> const& arrival_times ) const
  {
    const auto esop = create_sop_form( func );
    const auto [spp, sums] = kitty::simple_spp( esop, func.num_vars() );

    stopwatch<> t_tree( time_tree_balancing );
    std::vector<signal<Ntk>> and_terms;
    uint32_t max_level{};
    uint32_t num_and_gates{};
    for ( auto const& cube : spp )
    {
      arrival_time_queue<Ntk> product_queue;
      for ( auto i = 0u; i < func.num_vars(); ++i )
      {
        if ( cube.get_mask( i ) )
        {
          const auto [f, l] = arrival_times[i];
          product_queue.push( { cube.get_bit( i ) ? f : dest.create_not( f ), l } );
        }
      }
      for ( auto i = 0u; i < sums.size(); ++i )
      {
        if ( cube.get_mask( func.num_vars() + i ) )
        {
          std::vector<signal<Ntk>> xor_terms;
          uint32_t xor_level{};
          for ( auto j = 0u; j < func.num_vars(); ++j )
          {
            if ( ( sums[i] >> j ) & 1 )
            {
              const auto [f, l] = arrival_times[j];
              xor_terms.push_back( f );
              xor_level = std::max( xor_level, l );
            }
          }
          const auto f = dest.create_nary_xor( xor_terms );
          product_queue.push( { cube.get_bit( func.num_vars() + i ) ? f : dest.create_not( f ), xor_level } );
        }
      }
      if ( product_queue.size() )
      {
        num_and_gates += static_cast<uint32_t>( product_queue.size() ) - 1u;
      }
      auto [s, l] = balanced_and_tree( dest, product_queue );
      and_terms.push_back( s );
      max_level = std::max( max_level, l );
    }
    return { and_terms, max_level, num_and_gates };
  }

  arrival_time_pair<Ntk> balanced_and_tree( Ntk& dest, arrival_time_queue<Ntk>& queue ) const
  {
    if ( queue.empty() )
    {
      return { dest.get_constant( true ), 0u };
    }

    while ( queue.size() > 1u )
    {
      auto [s1, l1] = queue.top();
      queue.pop();
      auto [s2, l2] = queue.top();
      queue.pop();
      const auto s = dest.create_and( s1, s2 );
      const auto l = std::max( l1, l2 ) + 1;
      queue.push( { s, l } );
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
      return sop_hash_[func] = mockturtle::exorcism( func ); // TODO generalize
    }
  }

private:
  mutable std::unordered_map<kitty::dynamic_truth_table, std::vector<kitty::cube>, kitty::hash<kitty::dynamic_truth_table>> sop_hash_;

public:
  bool spp_optimization{ false };
  bool mux_optimization{ false };

public:
  mutable uint32_t sop_cache_hits{};
  mutable uint32_t sop_cache_misses{};

  mutable stopwatch<>::duration time_sop{};
  mutable stopwatch<>::duration time_tree_balancing{};
};

} // namespace mockturtle