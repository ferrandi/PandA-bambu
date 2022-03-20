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
  \file random_logic_generator.hpp
  \brief Generate random logic networks

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/xag.hpp>
#include <mockturtle/networks/mig.hpp>
#include <random>

namespace mockturtle
{

/*! \brief Generates a random logic network
 *
 * Generate a random logic network with certain parameters.
 *
 * The constructor takes a vector of construction rules, which are
 * used in the algorithm to build the logic network.
 *
 */
template<typename Ntk>
class random_logic_generator
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  struct rule
  {
    std::function<signal(Ntk&, std::vector<signal> const&)> func;
    uint32_t num_args;
  };

  using rules_t = std::vector<rule>;

public:
  explicit random_logic_generator( rules_t const &gens )
    : _gens( gens )
  {
  }

  /*! \brief Generate network
   *
   * Generate a random logic network with a fixed number of primary
   * inputs, a fixed number of gates, and an unrestricted number of
   * primary outputs.  After generating primary inputs and gates, all
   * nodes with no fanout become primary outputs.
   */
  Ntk generate( uint32_t num_inputs, uint32_t num_gates, uint64_t seed = 0xcafeaffe ) const
  {
    assert( num_inputs > 0 );
    assert( num_gates > 0 );

    std::vector<signal> fs;
    Ntk ntk;

    /* generate constant */
    fs.emplace_back( ntk.get_constant( false ) );

    /* generate pis */
    for ( auto i = 0u; i < num_inputs; ++i )
    {
      fs.emplace_back( ntk.create_pi() );
    }

    /* generate gates */
    std::mt19937 rng( static_cast<unsigned int>( seed ) );
    std::uniform_int_distribution<int> rule_dist( 0, static_cast<int>( _gens.size() - 1u ) );

    auto gate_counter = ntk.num_gates();
    while ( gate_counter < num_gates )
    {
      auto const r = _gens.at( rule_dist( rng ) );

      std::uniform_int_distribution<int> dist( 0, static_cast<int>( fs.size() - 1 ) );
      std::vector<signal> args;
      for ( auto i = 0u; i < r.num_args; ++i )
      {
        auto const a_compl = dist( rng ) & 1;
        auto const a = fs.at( dist( rng ) );
        args.emplace_back( a_compl ? !a : a );
      }

      auto const g = r.func( ntk, args );
      if ( ntk.num_gates() > gate_counter )
      {
        fs.emplace_back( g );
        ++gate_counter;
      }

      assert( ntk.num_gates() == gate_counter );
    }

    /* generate pos */
    ntk.foreach_node( [&]( auto const& n ){
        auto const size = ntk.fanout_size( n );
        if ( size == 0u )
        {
          ntk.create_po( ntk.make_signal( n ) );
        }
      });

    assert( ntk.num_pis() == num_inputs );
    assert( ntk.num_gates() == num_gates );

    return ntk;
  }

  /*! \brief Generate network
   *
   * Generate a random logic network with a fixed numbers of primary
   * inputs, primary outputs, and a suggestion for the structure in
   * form a list { l_1, ..., l_k }, where k denotes the number of
   * levels between primary inputs and primary outputs and each l_i, 1
   * <= 0 <= k, denotes a suggestion for the number of nodes at level
   * i.  Depending on the parameters some of the nodes in the networks
   * may be dangling.
   *
   * The method updates the argument `num_gates_per_level` with the
   * actual number of gates at each level.
   */
  Ntk generate2( uint32_t num_inputs, uint32_t num_outputs, std::vector<uint32_t> &num_gates_per_level, uint64_t seed = 0xcafeaffe ) const
  {
    assert( num_inputs > 0 );
    assert( num_outputs > 0 );
    assert( num_gates_per_level.size() >= 0 );

    uint64_t const num_levels{num_gates_per_level.size() + 2u};
    std::vector<std::vector<signal>> levels( num_levels );

    Ntk ntk;

    /* generate constant */
    levels[0].emplace_back( ntk.get_constant( false ) );

    /* generate pis */
    while ( ntk.num_pis() < num_inputs )
    {
      levels[0].emplace_back( ntk.create_pi() );
    }

    /* generate gates */
    std::default_random_engine generator( seed );
    std::uniform_int_distribution<uint32_t> rule_dist( 0, _gens.size() - 1u );

    uint64_t gate_counter{ntk.num_gates()};
    uint64_t curr_level{0u};

    while ( ++curr_level < num_levels )
    {
      uint64_t iterations{0};
      uint64_t const num_expected_gates{curr_level == num_levels - 1 ? num_outputs : num_gates_per_level[curr_level - 1u]};
      while ( levels[curr_level].size() < num_expected_gates && iterations++ < std::max( uint64_t(3ul*num_expected_gates), uint64_t(50ul) ) )
      {
        /* select a rule */
        auto const rule = _gens.at( rule_dist( generator ) );

        /* select an element */
        std::uniform_int_distribution<uint32_t> dist( 0, levels[curr_level - 1].size() - 1 );
        std::vector<signal> args;
        while ( args.size() < rule.num_args )
        {
          bool const s_compl = dist( generator ) & 1;
          signal const s = levels[curr_level - 1].at( dist( generator ) );
          args.emplace_back( s_compl ? !s : s );
        }

        /* apply rule */
        signal const gate = rule.func( ntk, args );
        if ( ntk.num_gates() > gate_counter )
        {
          levels[curr_level].emplace_back( gate );
          ++gate_counter;
        }
      }

      /* update num_gates_per_level */
      if ( curr_level < num_levels - 1 )
      {
        num_gates_per_level[curr_level - 1u] = levels[curr_level].size();
      }
    }

    /* generate pos */
    for ( auto& po : levels[num_levels - 1] )
    {
      ntk.create_po( po );
    }

    while ( ntk.num_pos() < num_outputs )
    {
      ntk.create_po( ntk.get_constant( false ) );
    }

    assert( ntk.num_pis() == num_inputs );
    assert( ntk.num_pos() == num_outputs );

    return ntk;
  }

  rules_t const _gens;
};

/*! \brief Generates a random AIG network */
inline random_logic_generator<aig_network> default_random_aig_generator()
{
  using gen_t = random_logic_generator<aig_network>;

  gen_t::rules_t rules;
  rules.emplace_back( gen_t::rule{[]( aig_network& aig, std::vector<aig_network::signal> const& vs ) -> aig_network::signal
    {
      assert( vs.size() == 2u );
      return aig.create_and( vs[0], vs[1] );
    }, 2u} );

  return gen_t( rules );
}

/*! \brief Generates a random XAG network */
inline random_logic_generator<xag_network> default_random_xag_generator()
{
  using gen_t = random_logic_generator<xag_network>;

  gen_t::rules_t rules;
  rules.emplace_back( gen_t::rule{[]( xag_network& xag, std::vector<xag_network::signal> const& vs ) -> xag_network::signal
    {
      assert( vs.size() == 2u );
      return xag.create_and( vs[0], vs[1] );
    }, 2u} );
  rules.emplace_back( gen_t::rule{[]( xag_network& xag, std::vector<xag_network::signal> const& vs ) -> xag_network::signal
    {
      assert( vs.size() == 2u );
      return xag.create_xor( vs[0], vs[1] );
    }, 2u} );

  return gen_t( rules );
}

/*! \brief Generates a random MIG network */
inline random_logic_generator<mig_network> default_random_mig_generator()
{
  using gen_t = random_logic_generator<mig_network>;

  gen_t::rules_t rules;
  rules.emplace_back( gen_t::rule{[]( mig_network& mig, std::vector<mig_network::signal> const& vs ) -> mig_network::signal
    {
      assert( vs.size() == 3u );
      return mig.create_maj( vs[0], vs[1], vs[2] );
    }, 3u} );

  return random_logic_generator<mig_network>( rules );
}

/*! \brief Generates a random MIG network MAJ-, AND-, and OR-gates */
inline random_logic_generator<mig_network> mixed_random_mig_generator()
{
  using gen_t = random_logic_generator<mig_network>;

  gen_t::rules_t rules;
  rules.emplace_back( gen_t::rule{[]( mig_network& mig, std::vector<mig_network::signal> const& vs ) -> mig_network::signal
    {
      assert( vs.size() == 3u );
      return mig.create_maj( vs[0], vs[1], vs[2] );
    }, 3u} );
  rules.emplace_back( gen_t::rule{[]( mig_network& mig, std::vector<mig_network::signal> const& vs ) -> mig_network::signal
    {
      assert( vs.size() == 2u );
      return mig.create_and( vs[0], vs[1] );
    }, 2u} );
  rules.emplace_back( gen_t::rule{[]( mig_network& mig, std::vector<mig_network::signal> const& vs ) -> mig_network::signal
    {
      assert( vs.size() == 2u );
      return mig.create_or( vs[0], vs[1] );
    }, 2u} );

  return random_logic_generator<mig_network>( rules );
}

} // namespace mockturtle