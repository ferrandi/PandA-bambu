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
  \file write_blif.hpp
  \brief Write networks to BLIF format

  \author Heinz Riener
*/

#pragma once

#include "../traits.hpp"
#include "../views/topo_view.hpp"

#include <kitty/constructors.hpp>
#include <kitty/isop.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>

#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <string>

namespace mockturtle
{

/*! \brief Writes network in BLIF format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required network functions:**
 * - `fanin_size`
 * - `foreach_fanin`
 * - `foreach_pi`
 * - `foreach_po`
 * - `get_node`
 * - `is_constant`
 * - `is_pi`
 * - `node_function`
 * - `node_to_index`
 * - `num_pis`
 * - `num_pos`
 *
 * \param ntk Network
 * \param os Output stream
 */
template<class Ntk>
void write_blif( Ntk const& ntk, std::ostream& os )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_fanin_size_v<Ntk>, "Ntk does not implement the fanin_size method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

  topo_view topo_ntk{ntk};

  /* write model */
  os << ".model netlist\n";

  /* write inputs */
  if ( topo_ntk.num_pis() > 0u )
  {
      os << ".inputs ";
      topo_ntk.foreach_pi( [&]( auto const& n ) {
          if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
          {
            signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.node_to_index( n ) );
            std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "c_{}", topo_ntk.get_node( s ) );
            os << name << ' ';
          }
          else
          {
            os << fmt::format( "c_n{} ", topo_ntk.node_to_index( n ) );
          }
        } );
      os << "\n";
  }

  /* write outputs */
  if ( topo_ntk.num_pos() > 0u )
  {
    os << ".outputs ";
    topo_ntk.foreach_po( [&]( auto const& f, auto index ) {
        (void)f;
        if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
        {
          std::string const output_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : fmt::format( "po_n{}", index );
          os << output_name << ' ';
        }
        else
        {
          os << fmt::format( "po{} ", index );
        }
      });
    os << "\n";
  }

  /* write constants */
  os << ".names c_n0\n";
  os << "0\n";

  if ( ntk.get_constant( false ) != ntk.get_constant( true ) )
  {
    os << ".names c_n1\n";
    os << "1\n";
  }

  /* write nodes */
  topo_ntk.foreach_node( [&]( auto const& n ) {
      if ( topo_ntk.is_constant( n ) || topo_ntk.is_pi( n ) )
        return; /* continue */

      os << fmt::format( ".names " );

      /* write fanins of node */
      topo_ntk.foreach_fanin( n, [&]( auto const& f ) {
          if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
          {
            signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.node_to_index( topo_ntk.get_node( f ) ) );
            std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "c_n{}", topo_ntk.get_node( s ) );
            os << name << ' ';
          }
          else
          {
            os << fmt::format( "c_n{} ", topo_ntk.node_to_index( topo_ntk.get_node( f ) ) );
          }
        });

      /* write fanout of node */
      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
      {
        auto const s = topo_ntk.make_signal( topo_ntk.node_to_index( n ) );
        std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "c_n{}", topo_ntk.get_node( s ) );
        os << name << '\n';
      }
      else
      {
        os << fmt::format( "c_n{}\n", topo_ntk.node_to_index( n ) );
      }

      /* write truth table of node */
      auto func = topo_ntk.node_function( n );

      for ( auto cube : isop( func ) )
      {
        topo_ntk.foreach_fanin( n, [&]( auto const& f, auto index ) {
            if ( cube.get_mask( index ) && topo_ntk.is_complemented( f ) )
              cube.flip_bit( index );
          });

        cube.print( topo_ntk.fanin_size( n ), os );
        os << " 1\n";
      }
    });

  if ( topo_ntk.num_pos() > 0u )
  {
    topo_ntk.foreach_po( [&]( auto const& f, auto index ){
        auto const minterm_string = topo_ntk.is_complemented( f ) ? "0" : "1";
        if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> && has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
        {
          signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.node_to_index( topo_ntk.get_node( f ) ) );
          std::string const node_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "c_n{}", topo_ntk.get_node( s ) );
          std::string const output_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : fmt::format( "po_n{}", index );
          os << fmt::format( ".names {} {}\n{} 1\n", node_name, output_name, minterm_string, index );
        }
        else
        {
          os << fmt::format( ".names c_n{} po{}\n{} 1\n", topo_ntk.node_to_index( topo_ntk.node_to_index( topo_ntk.get_node( f ) ) ), index, minterm_string );
        }
      });
  }

  os << ".end\n";
  os << std::flush;
}

/*! \brief Writes network in BLIF format into a file
 *
 * **Required network functions:**
 * - `fanin_size`
 * - `foreach_fanin`
 * - `foreach_pi`
 * - `foreach_po`
 * - `get_node`
 * - `is_constant`
 * - `is_pi`
 * - `node_function`
 * - `node_to_index`
 * - `num_pis`
 * - `num_pos`
 *
 * \param ntk Network
 * \param filename Filename
 */
template<class Ntk>
void write_blif( Ntk const& ntk, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_blif( ntk, os );
  os.close();
}

} /* namespace mockturtle */
