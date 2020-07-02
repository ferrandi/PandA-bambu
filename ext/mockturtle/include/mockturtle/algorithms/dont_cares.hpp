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
  \file dont_cares.hpp
  \brief Compute don't cares

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <vector>

#include "../algorithms/cnf.hpp"
#include "../algorithms/reconv_cut.hpp"
#include "../algorithms/simulation.hpp"
#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../utils/include/percy.hpp"
#include "../views/fanout_view.hpp"
#include "../views/topo_view.hpp"
#include "../views/window_view.hpp"

#include <fmt/format.h>
#include <kitty/bit_operations.hpp>
#include <kitty/dynamic_truth_table.hpp>

namespace mockturtle
{

/*! \brief Computes satisfiability don't cares of a set of nodes.
 *
 * This function returns an under approximation of input assignments that
 * cannot occur on a given set of nodes in a network.  They may therefore be
 * used as don't care conditions.
 *
 * \param ntk Network
 * \param leaves Set of nodes
 * \param max_tfi_inputs Maximum number of inputs in the transitive fanin.
 */
template<class Ntk>
kitty::dynamic_truth_table satisfiability_dont_cares( Ntk const& ntk, std::vector<node<Ntk>> const& leaves, uint32_t max_tfi_inputs = 16u )
{
  auto extended_leaves = reconv_cut( reconv_cut_params{max_tfi_inputs} )( ntk, leaves );
  
  fanout_view<Ntk> fanout_ntk{ntk};
  fanout_ntk.clear_visited();

  window_view<fanout_view<Ntk>> window_ntk{fanout_ntk, extended_leaves, leaves, false};

  default_simulator<kitty::dynamic_truth_table> sim( window_ntk.num_pis() );
  const auto tts = simulate_nodes<kitty::dynamic_truth_table>( window_ntk, sim );

  /* first create care and then invert */
  kitty::dynamic_truth_table care( static_cast<uint32_t>( leaves.size() ) );
  for ( auto i = 0u; i < ( 1u << window_ntk.num_pis() ); ++i )
  {
    uint32_t entry{0u};
    for ( auto j = 0u; j < leaves.size(); ++j )
    {
      entry |= kitty::get_bit( tts[leaves[j]], i ) << j;
    }
    kitty::set_bit( care, entry );
  }
  return ~care;
}

/*! \brief Computes observability don't cares of a node.
 *
 * This function returns input assignemnts for which a change of the
 * node's value cannot be observed at any of the roots.  They may
 * therefore be used as don't care conditions.
 *
 * \param ntk Network
 * \param node A node in the ntk
 * \param leaves Set of leave nodes
 * \param roots Set of root nodes
 */
template<class Ntk>
kitty::dynamic_truth_table observability_dont_cares( Ntk const& ntk, node<Ntk> const& n, std::vector<node<Ntk>> const& leaves, std::vector<node<Ntk>> const& roots )
{
  fanout_view<Ntk> fanout_ntk{ntk};
  fanout_ntk.clear_visited();

  window_view<fanout_view<Ntk>> window_ntk{fanout_ntk, leaves, roots, false};

  default_simulator<kitty::dynamic_truth_table> sim( window_ntk.num_pis() );
  unordered_node_map<kitty::dynamic_truth_table, Ntk> node_to_value0( ntk );
  unordered_node_map<kitty::dynamic_truth_table, Ntk> node_to_value1( ntk );

  node_to_value0[n] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( false ) ) ) );
  simulate_nodes( ntk, node_to_value0, sim );

  node_to_value1[n] = ~sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( false ) ) ) );
  simulate_nodes( ntk, node_to_value1, sim );

  kitty::dynamic_truth_table care( static_cast<uint32_t>( leaves.size() ) );
  for ( const auto& r : roots )
  {
    care |= node_to_value0[r] ^ node_to_value1[r];
  }
  return ~care;
}

/*! \brief SAT-based satisfiability don't cares checker
 *
 * Initialize this class with a network and then call `is_dont_care` on a node
 * to check whether the given assignment is a satisfiability don't care.
 *
 * The assignment is assumed to be directly at the inputs of the gate, not
 * taking into account possible complemented fanins.
 */
template<class Ntk>
struct satisfiability_dont_cares_checker
{
  explicit satisfiability_dont_cares_checker( Ntk const& ntk )
      : ntk_( ntk ),
        literals_( node_literals( ntk ) )
  {
    init();
  }

  bool is_dont_care( node<Ntk> const& n, std::vector<bool> const& assignment )
  {
    if ( ntk_.fanin_size( n ) != assignment.size() ) return false;

    std::vector<pabc::lit> assumptions( assignment.size() );
    ntk_.foreach_fanin( n, [&]( auto const& f, auto i ) {
      assumptions[i] = lit_not_cond( literals_[ntk_.get_node( f )], assignment[i] == ntk_.is_complemented( f )  );
    } );

    return solver_.solve( &assumptions[0], &assumptions[0] + assumptions.size(), 0 ) == percy::failure;
  }

private:
  void init()
  {
    generate_cnf<Ntk>( ntk_, [&]( auto const& clause ) {
      solver_.add_clause( clause );
    }, literals_ );
  }

private:
  Ntk const& ntk_;
  percy::bsat_wrapper solver_;
  node_map<uint32_t, Ntk> literals_;
};

} /* namespace mockturtle */
