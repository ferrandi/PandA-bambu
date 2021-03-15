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
  \file index_list.hpp
  \brief Network representation as index lists

  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../networks/aig.hpp"
#include "../networks/xag.hpp"
#include "../traits.hpp"

#include <fmt/format.h>

namespace mockturtle
{

/*! \brief Creates AND and XOR gates from binary index list.
 *
 * The `begin` iterator points to the first index of an index list that has the
 * following 32-bit unsigned integer elements.  It starts with a signature whose
 * partitioned into `| num_gates | num_pos | num_pis |`, where `num_gates`
 * accounts for the most-significant 16 bits, `num_pos` accounts for 8 bits, and
 * `num_pis` accounts for the least-significant 8 bits.  Afterwards, gates are
 * defined as literal indexes `(2 * i + c)`, where `i` is an index, with 0
 * indexing the constant 0, 1 to `num_pis` indexing the primary inputs, and all
 * successive indexes for the gates.  Gate literals come in pairs.  If the first
 * literal has a smaller value than the second one, an AND gate is created,
 * otherwise, an XOR gate is created.  Afterwards, all outputs are defined in
 * terms of literals.  The length of both the index list and the primary input
 * list can be derived from the signature, and therefore, no end iterators need
 * to be passed to this function.
 *
 * \param dest Network
 * \param begin Start iterator to index list
 * \param pi_begin Start iterator to primary inputs (in network)
 * \return Returns a vector of signals created based on index list
 *
 * Example: The following index list creates (x1 AND x2) XOR (x3 AND x4):
 * `{3 << 16 | 1 << 8 | 4, 2, 4, 6, 8, 12, 10, 14}`
 */
template<class Ntk, class IndexIterator, class LeavesIterator>
std::vector<signal<Ntk>> create_from_binary_index_list( Ntk& dest, IndexIterator begin, LeavesIterator pi_begin )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );

  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<IndexIterator>::value_type>, uint32_t>, "IndexIterator value_type must be uint32_t" );
  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<LeavesIterator>::value_type>, signal<Ntk>>, "LeavesIterator value_type must be Ntk signal type" );

  const auto signature = *begin++;
  const auto num_pis = signature & 0xff;
  const auto num_pos = ( signature >> 8 ) & 0xff;
  const auto num_gates = signature >> 16;

  // initialize gate-signal list
  std::vector<signal<Ntk>> fs;
  fs.push_back( dest.get_constant( false ) );
  std::copy_n( pi_begin, num_pis, std::back_inserter( fs ) );

  for ( auto i = 0u; i < num_gates; ++i )
  {
    const auto signal1 = *begin++;
    const auto signal2 = *begin++;

    const auto c1 = signal1 % 2 ? dest.create_not( fs[signal1 / 2] ) : fs[signal1 / 2];
    const auto c2 = signal2 % 2 ? dest.create_not( fs[signal2 / 2] ) : fs[signal2 / 2];

    fs.push_back( signal1 > signal2 ? dest.create_xor( c1, c2 ) : dest.create_and( c1, c2 ) );
  }

  std::vector<signal<Ntk>> pos( num_pos );
  for ( auto i = 0u; i < num_pos; ++i )
  {
    const auto signal = *begin++;
    pos[i] = signal % 2 ? dest.create_not( fs[signal / 2] ) : fs[signal / 2];
  }

  return pos;
}

/*! \brief Creates AND and XOR gates from binary index list.
 *
 * An out-of-place variant for create_from_binary_index_list.
 */
template<class Ntk, class IndexIterator>
Ntk create_from_binary_index_list( IndexIterator begin )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );

  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<IndexIterator>::value_type>, uint32_t>, "IndexIterator value_type must be uint32_t" );

  Ntk ntk;
  std::vector<signal<Ntk>> pis( *begin & 0xff );
  std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );
  for ( auto const& f : create_from_binary_index_list<Ntk>( ntk, begin, pis.begin() ) )
  {
    ntk.create_po( f );
  }
  return ntk;
}

namespace detail
{

template<class Ntk>
std::vector<uint32_t> to_index_list( Ntk const& ntk )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network> || std::is_same_v<typename Ntk::base_type, aig_network>, "Ntk must be XAG or AIG" );

  std::vector<uint32_t> index_list;
  index_list.push_back( ( ntk.num_gates() << 16 ) | ( ntk.num_pos() << 8 ) | ntk.num_pis() );

  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    if ( ntk.node_to_index( n ) != i + 1 ) {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", i + 1 );
      std::abort();
    }
  });

  ntk.foreach_gate( [&]( auto const& n, auto i ) {
    if ( ntk.node_to_index( n ) != ntk.num_pis() + i + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.node_to_index( ntk.get_node( f ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      index_list.push_back( 2 * ntk.node_to_index( ntk.get_node( f ) ) + ( ntk.is_complemented( f ) ? 1 : 0 ) );
    } );
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    index_list.push_back( 2 * ntk.node_to_index( ntk.get_node( f ) ) + ( ntk.is_complemented( f ) ? 1 : 0 ) );
  });

  return index_list;
}

template<class Ntk>
std::string to_index_list_string( Ntk const& ntk )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network> || std::is_same_v<typename Ntk::base_type, aig_network>, "Ntk must be XAG or AIG" );

  /* compute signature */
  auto s = fmt::format( "{{{} << 16 | {} << 8 | {}", ntk.num_gates(), ntk.num_pos(), ntk.num_pis() );

  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    if ( ntk.node_to_index( n ) != i + 1 ) {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", i + 1 );
      std::abort();
    }
  });

  ntk.foreach_gate( [&]( auto const& n, auto i ) {
    if ( ntk.node_to_index( n ) != ntk.num_pis() + i + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.node_to_index( ntk.get_node( f ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      s += fmt::format( ", {}", 2 * ntk.node_to_index( ntk.get_node( f ) ) + ( ntk.is_complemented( f ) ? 1 : 0 ) );
    } );
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    s += fmt::format( ", {}", 2 * ntk.node_to_index( ntk.get_node( f ) ) + ( ntk.is_complemented( f ) ? 1 : 0 ) );
  });

  s += "}";

  return s;
}

}

} /* namespace mockturtle */
