/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2021  EPFL
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
  \file write_aiger.hpp
  \brief Write networks to AIGER format

  \author Heinz Riener

  A detailed description of the (binary) AIGER format and its encoding is available at [1].

  [1] http://fmv.jku.at/aiger/
*/

#pragma once

#include "../traits.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>

namespace mockturtle
{

namespace detail
{

void encode( std::vector<unsigned char>& buffer, uint32_t lit )
{
  unsigned char ch;
  while ( lit & ~0x7f )
  {
    ch = ( lit & 0x7f ) | 0x80;
    buffer.push_back( ch );
    lit >>= 7;
  }
  ch = lit;
  buffer.push_back( ch );
}

} /* detail */

/*! \brief Writes a combinational AIG network in binary AIGER format into a file
 *
 * **Required network functions:**
 * - `num_cis`
 * - `num_cos`
 * - `foreach_gate`
 * - `foreach_fanin`
 * - `foreach_po`
 * - `get_node`
 * - `is_complemented`
 *
 * \param aig Combinational AIG network
 * \param os Output stream
 */
void write_aiger( aig_network const& aig, std::ostream& os )
{
  static_assert( is_network_type_v<aig_network>, "Ntk is not a network type" );
  static_assert( has_num_cis_v<aig_network>, "Ntk does not implement the num_cis method" );
  static_assert( has_num_cos_v<aig_network>, "Ntk does not implement the num_cos method" );
  static_assert( has_foreach_gate_v<aig_network>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<aig_network>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<aig_network>, "Ntk does not implement the foreach_po method" );
  static_assert( has_get_node_v<aig_network>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<aig_network>, "Ntk does not implement the is_complemented method" );

  assert( aig.is_combinational() && "Network has to be combinational" );

  using node = aig_network::node;
  using signal = aig_network::signal;

  assert( aig.num_latches() == 0u );
  uint32_t const M = aig.num_cis() + aig.num_gates() + aig.num_latches();

  /* HEADER */
  char string_buffer[1024];
  sprintf( string_buffer, "aig %u %u %u %u %u\n", M, aig.num_pis(), aig.num_latches(), aig.num_pos(), aig.num_gates() );
  os.write( &string_buffer[0], sizeof( unsigned char )*strlen( string_buffer ) );

  /* POs */
  aig.foreach_po( [&]( signal const& f ){
    sprintf( string_buffer, "%u\n", uint32_t(2*aig.get_node( f ) + aig.is_complemented( f )) );
    os.write( &string_buffer[0], sizeof( unsigned char )*strlen( string_buffer ) );
  });

  /* GATES */
  std::vector<unsigned char> buffer;
  aig.foreach_gate( [&]( node const& n ){
    std::vector<uint32_t> lits;
    lits.push_back( 2*n );

    aig.foreach_fanin( n, [&]( signal const& fi ){
      lits.push_back( 2*aig.get_node( fi ) + aig.is_complemented( fi ) );
    });

    if ( lits[1] > lits[2] )
    {
      auto const tmp = lits[1];
      lits[1] = lits[2];
      lits[2] = tmp;
    }

    assert( lits[2] < lits[0] );
    detail::encode( buffer, lits[0] - lits[2] );
    detail::encode( buffer, lits[2] - lits[1] );
  });

  for ( const auto& b : buffer )
  {
    os.put( b );
  }

  /* COMMENT */
  os.put( 'c' );
}

/*! \brief Writes a combinational AIG network in binary AIGER format into a file
 *
 * **Required network functions:**
 * - `num_cis`
 * - `num_cos`
 * - `foreach_gate`
 * - `foreach_fanin`
 * - `foreach_po`
 * - `get_node`
 * - `is_complemented`
 *
 * \param aig Combinational AIG network
 * \param filename Filename
 */
void write_aiger( aig_network const& aig, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_aiger( aig, os );
  os.close();
}

} /* namespace mockturtle */