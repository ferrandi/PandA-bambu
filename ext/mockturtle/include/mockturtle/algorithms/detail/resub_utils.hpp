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
  \file resub_utils.hpp
  \brief Utility classes for the resubstitution framework

  class `node_mffc_inside`, class `window_simulator` (originally `simulator`),
  and class `default_resub_functor` moved from resubstitution.hpp

  \author Heinz Riener
*/

#pragma once

#include <vector>
#include <optional>
#include <iostream>

#include <kitty/constructors.hpp>

namespace mockturtle::detail
{

/* based on abcRefs.c */
template<typename Ntk>
class node_mffc_inside
{
public:
  using node = typename Ntk::node;

public:
  explicit node_mffc_inside( Ntk const& ntk )
      : ntk( ntk )
  {
  }

  int32_t run( node const& n, std::vector<node> const& leaves, std::vector<node>& inside )
  {
    /* increment the fanout counters for the leaves */
    for ( const auto& l : leaves )
      ntk.incr_fanout_size( l );

    /* dereference the node */
    auto count1 = node_deref_rec( n );

    /* collect the nodes inside the MFFC */
    node_mffc_cone( n, inside );

    /* reference it back */
    auto count2 = node_ref_rec( n );
    (void)count2;
    assert( count1 == count2 );

    for ( const auto& l : leaves )
      ntk.decr_fanout_size( l );

    return count1;
  }

private:
  /* ! \brief Dereference the node's MFFC */
  int32_t node_deref_rec( node const& n )
  {
    if ( ntk.is_pi( n ) )
      return 0;

    int32_t counter = 1;
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk.get_node( f );

      ntk.decr_fanout_size( p );
      if ( ntk.fanout_size( p ) == 0 )
      {
        counter += node_deref_rec( p );
      }
    } );

    return counter;
  }

  /* ! \brief Reference the node's MFFC */
  int32_t node_ref_rec( node const& n )
  {
    if ( ntk.is_pi( n ) )
      return 0;

    int32_t counter = 1;
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk.get_node( f );

      auto v = ntk.fanout_size( p );
      ntk.incr_fanout_size( p );
      if ( v == 0 )
      {
        counter += node_ref_rec( p );
      }
    } );

    return counter;
  }

  void node_mffc_cone_rec( node const& n, std::vector<node>& cone, bool top_most )
  {
    /* skip visited nodes */
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      return;
    }
    ntk.set_visited( n, ntk.trav_id() );

    if ( !top_most && ( ntk.is_pi( n ) || ntk.fanout_size( n ) > 0 ) )
    {
      return;
    }

    /* recurse on children */
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      node_mffc_cone_rec( ntk.get_node( f ), cone, false );
    } );

    /* collect the internal nodes */
    cone.emplace_back( n );
  }

  void node_mffc_cone( node const& n, std::vector<node>& cone )
  {
    cone.clear();
    ntk.incr_trav_id();
    node_mffc_cone_rec( n, cone, true );
  }

private:
  Ntk const& ntk;
}; /* node_mffc_inside */

template<typename Ntk, typename TT>
class window_simulator
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using truthtable_t = TT;

  explicit window_simulator( Ntk const& ntk, uint32_t num_divisors, uint32_t max_pis )
      : ntk( ntk ), num_divisors( num_divisors ), tts( num_divisors + 1 ), node_to_index( ntk.size(), 0u ), phase( ntk.size(), false )
  {
    auto tt = kitty::create<truthtable_t>( max_pis );
    tts[0] = tt;

    for ( auto i = 0u; i < tt.num_vars(); ++i )
    {
      kitty::create_nth_var( tt, i );
      tts[i + 1] = tt;
    }
  }

  void resize()
  {
    if ( ntk.size() > node_to_index.size() )
    {
      node_to_index.resize( ntk.size(), 0u );
    }
    if ( ntk.size() > phase.size() )
    {
      phase.resize( ntk.size(), false );
    }
  }

  void assign( node const& n, uint32_t index )
  {
    assert( n < node_to_index.size() );
    assert( index < num_divisors + 1 );
    node_to_index[n] = index;
  }

  truthtable_t get_tt( signal const& s ) const
  {
    auto const tt = tts.at( node_to_index.at( ntk.get_node( s ) ) );
    return ntk.is_complemented( s ) ? ~tt : tt;
  }

  void set_tt( uint32_t index, truthtable_t const& tt )
  {
    tts[index] = tt;
  }

  void normalize( std::vector<node> const& nodes )
  {
    for ( const auto& n : nodes )
    {
      assert( n < phase.size() );
      assert( n < node_to_index.size() );

      if ( n == 0 )
      {
        return;
      }

      auto& tt = tts[node_to_index.at( n )];
      if ( kitty::get_bit( tt, 0 ) )
      {
        tt = ~tt;
        phase[n] = true;
      }
      else
      {
        phase[n] = false;
      }
    }
  }

  bool get_phase( node const& n ) const
  {
    assert( n < phase.size() );
    return phase.at( n );
  }

private:
  Ntk const& ntk;
  uint32_t num_divisors;

  std::vector<truthtable_t> tts;
  std::vector<uint32_t> node_to_index;
  std::vector<bool> phase;
}; /* window_simulator */

struct default_resub_functor_stats
{
  /*! \brief Accumulated runtime for const-resub */
  stopwatch<>::duration time_resubC{0};

  /*! \brief Accumulated runtime for zero-resub */
  stopwatch<>::duration time_resub0{0};

  /*! \brief Number of accepted constant resubsitutions */
  uint32_t num_const_accepts{0};

  /*! \brief Number of accepted zero resubsitutions */
  uint32_t num_div0_accepts{0};

  void report() const
  {
    std::cout << "[i] kernel: default_resub_functor\n";
    std::cout << fmt::format( "[i]     constant-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_const_accepts, to_seconds( time_resubC ) );
    std::cout << fmt::format( "[i]            0-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div0_accepts, to_seconds( time_resub0 ) );
    std::cout << fmt::format( "[i]            total   {:6d}\n",
                              ( num_const_accepts + num_div0_accepts ) );
  }
};

/*! \brief A window-based resub functor which is basically doing functional reduction (fraig). */
template<typename Ntk, typename Simulator, typename TT>
class default_resub_functor
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using stats = default_resub_functor_stats;

  explicit default_resub_functor( Ntk const& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, default_resub_functor_stats& st )
      : ntk( ntk ), sim( sim ), divs( divs ), num_divs( num_divs ), st( st )
  {
  }

  std::optional<signal> operator()( node const& root, TT care, uint32_t required, uint32_t max_inserts, uint32_t num_mffc, uint32_t& last_gain ) const
  {
    /* The default resubstitution functor does not insert any gates
       and consequently does not use the argument `max_inserts`. Other
       functors, however, make use of this argument. */
    (void)care;
    (void)max_inserts;
    assert( kitty::is_const0( ~care ) );

    /* consider constants */
    auto g = call_with_stopwatch( st.time_resubC, [&]() {
      return resub_const( root, required );
    } );
    if ( g )
    {
      ++st.num_const_accepts;
      last_gain = num_mffc;
      return g; /* accepted resub */
    }

    /* consider equal nodes */
    g = call_with_stopwatch( st.time_resub0, [&]() {
      return resub_div0( root, required );
    } );
    if ( g )
    {
      ++st.num_div0_accepts;
      last_gain = num_mffc;
      return g; /* accepted resub */
    }

    return std::nullopt;
  }

private:
  std::optional<signal> resub_const( node const& root, uint32_t required ) const
  {
    (void)required;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    if ( tt == sim.get_tt( ntk.get_constant( false ) ) )
    {
      return sim.get_phase( root ) ? ntk.get_constant( true ) : ntk.get_constant( false );
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div0( node const& root, uint32_t required ) const
  {
    (void)required;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    for ( const auto& d : divs )
    {
      if ( root == d )
      {
        break;
      }

      if ( tt != sim.get_tt( ntk.make_signal( d ) ) )
      {
        continue; /* next */
      }

      return ( sim.get_phase( d ) ^ sim.get_phase( root ) ) ? !ntk.make_signal( d ) : ntk.make_signal( d );
    }

    return std::nullopt;
  }

private:
  Ntk const& ntk;
  Simulator const& sim;
  std::vector<node> const& divs;
  uint32_t num_divs;
  stats& st;
}; /* default_resub_functor */

} /* namespace mockturtle::detail */