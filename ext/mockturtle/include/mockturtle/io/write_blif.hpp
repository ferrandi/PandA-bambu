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

  struct write_blif_params
  {
    uint32_t skip_feedthrough = 0u;
  };

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
void write_blif( Ntk const& ntk, std::ostream& os, write_blif_params const& ps = {} )
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
  os << ".model top\n";

  /* write inputs */
  if ( topo_ntk.num_pis() > 0u )
  {
    os << ".inputs ";
    topo_ntk.foreach_ci( [&]( auto const& n, auto index ) 
    {
      if ( ( ( index + 1 ) <= topo_ntk.num_cis() - topo_ntk.num_latches() ) ) 
      {
        if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
        {
          signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.node_to_index( n ) );
          std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "pi{}", topo_ntk.get_node( s ) );
          os << name << ' ';
        }
        else
        {
          os << fmt::format( "pi{} ", topo_ntk.node_to_index( n ) );
        }
      }
    } );
    os << "\n";
  }

  /* write outputs */
  if ( topo_ntk.num_pos() > 0u )
  {
    os << ".outputs ";
    topo_ntk.foreach_co( [&]( auto const& f, auto index ) 
    {
      (void)f;
      if( index < topo_ntk.num_cos() - topo_ntk.num_latches() ) 
      {
        if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
        {
          std::string const output_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : fmt::format( "po{}", index );
          os << output_name << ' ';
        }
        else
        {
          os << fmt::format( "po{} ", index );
        }
      }
    } );
    os << "\n";
  }

  if ( topo_ntk.num_latches() > 0u )
  {
    auto latch_idx = 0;
    topo_ntk.foreach_co( [&]( auto const& f, auto index ) 
    {
      if( index >= topo_ntk.num_cos() - topo_ntk.num_latches() ) 
      {
        os << ".latch ";
        auto const ro_sig = topo_ntk.make_signal( topo_ntk.ri_to_ro( f ) );
        mockturtle::latch_info l_info = topo_ntk._storage->latch_information[topo_ntk.get_node(ro_sig)];
        if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
        {
          std::string const ri_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : fmt::format( "new_n{}", topo_ntk.get_node( f ) );
          std::string const ro_name = topo_ntk.has_name( ro_sig ) ? topo_ntk.get_name( ro_sig ) : fmt::format( "new_n{}", topo_ntk.get_node( ro_sig ) );
          os << fmt::format( "{} {} {} {} {}\n", ri_name, ro_name, l_info.type, l_info.control, l_info.init);
        }
        else
        {
          os << fmt::format( "li{} new_n{} {} {} {}\n", latch_idx, topo_ntk.get_node( ro_sig ), l_info.type, l_info.control, l_info.init );
          latch_idx++;
        }
      }
    } );
  }

  /* write constants */
  os << ".names new_n0\n";
  os << "0\n";

  if ( topo_ntk.get_constant( false ) != topo_ntk.get_constant( true ) ) 
  {
    os << ".names new_n1\n";
    os << "1\n";
  }

  /* write nodes */
  topo_ntk.foreach_node( [&]( auto const& n ) 
  {

    if ( topo_ntk.is_constant( n ) || topo_ntk.is_ci( n ) )
      return; /* continue */

    /* write truth table of node */
    auto func = topo_ntk.node_function( n );

    if(isop( func ).size() == 0) 
    {
      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
      {
        auto const s = topo_ntk.make_signal( n );
        std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "new_n{}", topo_ntk.get_node( s ) );
        os << fmt::format( ".names {}\n", name );
        os << "0" << '\n';
      }
      else
      {
        os << fmt::format( ".names new_n{}\n", n );
        os << "0" << '\n';
      }
      return;
    }

    os << fmt::format( ".names " );

    /* write fanins of node */
    topo_ntk.foreach_fanin( n, [&]( auto const& f ) 
    {
      auto f_node = topo_ntk.get_node( f );
      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
      {
        signal<Ntk> const s = topo_ntk.make_signal( f_node );
        std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : topo_ntk.is_pi( f_node ) ? fmt::format( "pi{} ", f_node ) : fmt::format( "new_n{} ", f_node );
        os << name << ' ';
      }
      else
      {
        std::string const name = topo_ntk.is_pi( f_node ) ? fmt::format( "pi{} ", f_node ) : fmt::format( "new_n{} ", f_node );
        os << name;
      }
    });

    /* write fanout of node */
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
    {
      auto const s = topo_ntk.make_signal( n );
      std::string const name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "new_n{}", topo_ntk.get_node( s ) );
      os << name << '\n';
    }
    else
    {
      os << fmt::format( "new_n{}\n", n );
    }


    int count = 0;
    for ( auto cube : isop( func ) )
    {
      topo_ntk.foreach_fanin( n, [&]( auto const& f, auto index ) 
      {
        if ( cube.get_mask( index ) && topo_ntk.is_complemented( f ) )
          cube.flip_bit( index );
      });

      cube.print( topo_ntk.fanin_size( n ), os );
      os << " 1\n";
      count++;
    }
  } );

  auto latch_idx = 0;
  topo_ntk.foreach_co( [&]( auto const& f, auto index )
  {
    auto f_node = topo_ntk.get_node( f );
    auto const minterm_string = topo_ntk.is_complemented( f ) ? "0" : "1";
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> && has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
    {
      signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.get_node( f ) );
      std::string const node_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "new_n{}", topo_ntk.get_node( s ) );
      std::string const output_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : fmt::format( "po{}", index );
      if(!ps.skip_feedthrough || ( node_name != output_name ) )
        os << fmt::format( ".names {} {}\n{} 1\n", node_name, output_name, minterm_string, index );
    }
    else
    {
      if( index >= topo_ntk.num_cos() - topo_ntk.num_latches() ) 
      {
        if(!ps.skip_feedthrough || ( topo_ntk.get_node( f ) != index)){
          os << fmt::format( ".names new_n{} li{}\n{} 1\n", f_node, latch_idx, minterm_string );
          latch_idx++;
        }
      }
      else
      {
        std::string const node_name = topo_ntk.is_pi( f_node ) ? fmt::format( "pi{}", f_node ) : fmt::format( "new_n{}", f_node );
        if(!ps.skip_feedthrough ||  ( topo_ntk.get_node( f ) != index ) )
          os << fmt::format( ".names {} po{}\n{} 1\n", node_name, index, minterm_string );
      }
      
    }
  } );

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
void write_blif( Ntk const& ntk, std::string const& filename, write_blif_params const& ps = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_blif( ntk, os, ps );
  os.close();
}

} /* namespace mockturtle */
