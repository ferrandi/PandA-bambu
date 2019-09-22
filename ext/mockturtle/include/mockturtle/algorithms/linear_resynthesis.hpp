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
  \file linear_resynthesis.hpp
  \brief Resynthesize linear circuit

  \author Mathias Soeken
*/

#pragma once

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../algorithms/simulation.hpp"
#include "../networks/xag.hpp"
#include "../traits.hpp"

#include <fmt/format.h>

namespace mockturtle
{

namespace detail
{

class linear_sum_simulator
{
public:
  std::vector<uint32_t> compute_constant( bool ) const { return {}; }
  std::vector<uint32_t> compute_pi( uint32_t index ) const { return {index}; }
  std::vector<uint32_t> compute_not( std::vector<uint32_t> const& value ) const { return value; }
};

class linear_sum_xag : public xag_network
{
public:
  linear_sum_xag( xag_network const& xag ) : xag_network( xag ) {}

  template<typename Iterator>
  iterates_over_t<Iterator, std::vector<uint32_t>>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_pi( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto set1 = *begin++;
    auto set2 = *begin++;

    if ( c1.index < c2.index )
    {
      assert( false );
      return {};
    }
    else
    {
      std::vector<uint32_t> result;
      auto it1 = set1.begin();
      auto it2 = set2.begin();

      while ( it1 != set1.end() && it2 != set2.end() )
      {
        if ( *it1 < *it2 )
        {
          result.push_back( *it1++ );
        }
        else if ( *it1 > *it2 )
        {
          result.push_back( *it2++ );
        }
        else
        {
          ++it1;
          ++it2;
        }
      }

      if ( it1 != set1.end() )
      {
        std::copy( it1, set1.end(), std::back_inserter( result ) );
      }
      else if ( it2 != set2.end() )
      {
        std::copy( it2, set2.end(), std::back_inserter( result ) );
      }

      return result;
    }
  }
};

struct pair_hash
{
  template<class T1, class T2>
  std::size_t operator()( std::pair<T1, T2> const& p ) const
  {
    return std::hash<T1>()( p.first ) ^ std::hash<T2>()( p.second );
  }
};

template<class Ntk>
struct linear_resynthesis_paar_impl
{
public:
  using index_pair_t = std::pair<uint32_t, uint32_t>;

  linear_resynthesis_paar_impl( Ntk const& xag ) : xag( xag ) {}

  Ntk run()
  {
    xag.foreach_pi( [&]( auto const& ) {
      signals.push_back( dest.create_pi() );
    } );

    extract_linear_equations();

    while ( !occurrance_to_pairs.empty() )
    {
      const auto p = *( occurrance_to_pairs.back().begin() );
      replace_one_pair( p );
    }

    xag.foreach_po( [&]( auto const& f, auto i ) {
      if ( linear_equations[i].empty() )
      {
        dest.create_po( dest.get_constant( xag.is_complemented( f ) ) );
      }
      else
      {
        assert( linear_equations[i].size() == 1u );
        dest.create_po( signals[linear_equations[i].front()] ^ xag.is_complemented( f ) );
      }
    } );

    return dest;
  }

private:
  void extract_linear_equations()
  {
    occurrance_to_pairs.resize( 1u );

    linear_sum_xag lxag{xag};
    linear_equations = simulate<std::vector<uint32_t>>( lxag, linear_sum_simulator{} );

    for ( auto o = 0u; o < linear_equations.size(); ++o )
    {
      const auto& lin_eq = linear_equations[o];
      for ( auto j = 1u; j < lin_eq.size(); ++j )
      {
        for ( auto i = 0u; i < j; ++i )
        {
          const auto p = std::make_pair( lin_eq[i], lin_eq[j] );
          pairs_to_output[p].push_back( o );
          add_pair( p );
        }
      }
    }
  }

  void add_pair( index_pair_t const& p )
  {
    if ( auto it = pair_to_occurrance.find( p ); it != pair_to_occurrance.end() )
    {
      // found another time
      const auto occ = it->second;
      occurrance_to_pairs[occ - 1u].erase( p );
      if ( occurrance_to_pairs.size() <= occ + 1u )
      {
        occurrance_to_pairs.resize( occ + 1u );
      }
      occurrance_to_pairs[occ].insert( p );
      it->second++;
    }
    else
    {
      // first time found
      pair_to_occurrance[p] = 1u;
      occurrance_to_pairs[0u].insert( p );
    }
  }

  void remove_all_pairs( index_pair_t const& p )
  {
    auto it = pair_to_occurrance.find( p );
    const auto occ = it->second;
    pair_to_occurrance.erase( it );
    occurrance_to_pairs[occ - 1u].erase( p );
    while ( !occurrance_to_pairs.empty() && occurrance_to_pairs.back().empty() )
    {
      occurrance_to_pairs.pop_back();
    }
    pairs_to_output.erase( p );
  }

  void remove_one_pair( index_pair_t const& p, uint32_t output )
  {
    auto it = pair_to_occurrance.find( p );
    const auto occ = it->second;
    occurrance_to_pairs[occ - 1u].erase( p );
    if ( occ > 1u )
    {
      occurrance_to_pairs[occ - 2u].insert( p );
    }
    it->second--;
    pairs_to_output[p].erase( std::remove( pairs_to_output[p].begin(), pairs_to_output[p].end(), output ), pairs_to_output[p].end() );
  }

  void replace_one_pair( index_pair_t const& p )
  {
    const auto [a, b] = p;
    auto c = signals.size();
    signals.push_back( dest.create_xor( signals[a], signals[b] ) );

    /* update data structures */
    for ( auto o : pairs_to_output[p] )
    {
      auto& leq = linear_equations[o];
      leq.erase( std::remove( leq.begin(), leq.end(), a ), leq.end() );
      leq.erase( std::remove( leq.begin(), leq.end(), b ), leq.end() );
      for ( auto i : leq )
      {
        remove_one_pair( {std::min( i, a ), std::max( i, a )}, o );
        remove_one_pair( {std::min( i, b ), std::max( i, b )}, o );
        add_pair( {i, c} );
        pairs_to_output[{i, c}].push_back( o );
      }
      leq.push_back( c );
    }
    remove_all_pairs( p );
  }

  void print_linear_matrix()
  {
    for ( auto const& le : linear_equations )
    {
      auto it = le.begin();
      for ( auto i = 0u; i < signals.size(); ++i )
      {
        if ( it != le.end() && *it == i )
        {
          std::cout << " 1";
          it++;
        }
        else
        {
          std::cout << " 0";
        }
      }
      assert( it == le.end() );
      std::cout << "\n";
    }
  }

private:
  Ntk const& xag;
  Ntk dest;
  std::vector<signal<Ntk>> signals;
  std::vector<std::vector<uint32_t>> linear_equations;
  std::vector<std::unordered_set<index_pair_t, pair_hash>> occurrance_to_pairs;
  std::unordered_map<index_pair_t, uint32_t, pair_hash> pair_to_occurrance;
  std::unordered_map<index_pair_t, std::vector<uint32_t>, pair_hash> pairs_to_output;
};

} // namespace detail

/*! \brief Linear circuit resynthesis (Paar's algorithm)
 *
 * This algorithm works on an XAG that is only composed of XOR gates.  It
 * extracts a matrix representation of the linear output equations and
 * resynthesizes them in a greedy manner by always substituting the most
 * frequent pair of variables using the computed function of an XOR gate.
 *
 * Reference: [C. Paar, IEEE Int'l Symp. on Inf. Theo. (1997), page 250]
 */
template<typename Ntk>
Ntk linear_resynthesis_paar( Ntk const& xag )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Ntk is not XAG-like" );

  return detail::linear_resynthesis_paar_impl<Ntk>( xag ).run();
}

} /* namespace mockturtle */
