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
  \file simulation.hpp
  \brief Simulate networks

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <vector>

#include "../traits.hpp"
#include "../utils/node_map.hpp"

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/static_truth_table.hpp>

namespace mockturtle
{

/*! \brief Abstract template class for simulation. */
template<class SimulationType>
class default_simulator
{
public:
  default_simulator() = delete;
};

/*! \brief Simulates Boolean assignments.
 *
 * This simulator simulates Boolean values.  A vector with assignments for each
 * primary input must be passed to the constructor.
 */
template<>
class default_simulator<bool>
{
public:
  default_simulator() = delete;
  default_simulator( std::vector<bool> const& assignments ) : assignments( assignments ) {}

  bool compute_constant( bool value ) const { return value; }
  bool compute_pi( uint32_t index ) const { return assignments[index]; }
  bool compute_not( bool value ) const { return !value; }

private:
  std::vector<bool> assignments;
};

/*! \brief Simulates Boolean assignments with input word.
 *
 * This simulator simulates Boolean values.  A bitstring with assignments for
 * each primary input must be passed to the constructor.  Because this
 * bitstring can have at most 64 bits, this simulator is not suitable for
 * logic networks with more than 64 primary inputs.
 */
class input_word_simulator
{
public:
  input_word_simulator( uint64_t word ) : word( word ) {}

  bool compute_constant( bool value ) const { return value; }
  bool compute_pi( uint32_t index ) const { return ( word >> index ) & 1; }
  bool compute_not( bool value ) const { return !value; }

private:
  uint64_t word;
};

/*! \brief Simulates truth tables.
 *
 * This simulator simulates truth tables.  Each primary input is assigned the
 * projection function according to the index.  The number of variables be
 * passed to the constructor of the simulator.
 */
template<>
class default_simulator<kitty::dynamic_truth_table>
{
public:
  default_simulator() = delete;
  default_simulator( unsigned num_vars ) : num_vars( num_vars ) {}

  kitty::dynamic_truth_table compute_constant( bool value ) const
  {
    kitty::dynamic_truth_table tt( num_vars );
    return value ? ~tt : tt;
  }

  kitty::dynamic_truth_table compute_pi( uint32_t index ) const
  {
    kitty::dynamic_truth_table tt( num_vars );
    kitty::create_nth_var( tt, index );
    return tt;
  }

  kitty::dynamic_truth_table compute_not( kitty::dynamic_truth_table const& value ) const
  {
    return ~value;
  }

private:
  unsigned num_vars;
};

/*! \brief Simulates truth tables.
 *
 * This simulator simulates truth tables.  Each primary input is assigned the
 * projection function according to the index.  The number of variables must be
 * known at compile time.
 */
template<int NumVars>
class default_simulator<kitty::static_truth_table<NumVars>>
{
public:
  kitty::static_truth_table<NumVars> compute_constant( bool value ) const
  {
    kitty::static_truth_table<NumVars> tt;
    return value ? ~tt : tt;
  }

  kitty::static_truth_table<NumVars> compute_pi( uint32_t index ) const
  {
    kitty::static_truth_table<NumVars> tt;
    kitty::create_nth_var( tt, index );
    return tt;
  }

  kitty::static_truth_table<NumVars> compute_not( kitty::static_truth_table<NumVars> const& value ) const
  {
    return ~value;
  }
};

/*! \brief Simulates a network with a generic simulator.
 *
 * This is a generic simulation algorithm that can simulate arbitrary values.
 * In order to that, the network needs to implement the `compute` method for
 * `SimulationType` and one must pass an instance of a `Simulator` that
 * implements the three methods:
 * - `SimulationType compute_constant(bool)`
 * - `SimulationType compute_pi(index)`
 * - `SimulationType compute_not(SimulationType const&)`
 *
 * The method `compute_constant` returns a simulation value for a constant
 * value.  The method `compute_pi` returns a simulation value for a primary
 * input based on its index, and `compute_not` to invert a simulation value.
 *
 * This method returns a map that maps each node to its computed simulation
 * value.
 *
 * **Required network functions:**
 * - `foreach_po`
 * - `get_constant`
 * - `constant_value`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_gate`
 * - `fanin_size`
 * - `num_pos`
 * - `compute<SimulationType>`
 *
 * \param ntk Network
 * \param sim Simulator, which implements the simulator interface
 */
template<class SimulationType, class Ntk, class Simulator = default_simulator<SimulationType>>
node_map<SimulationType, Ntk> simulate_nodes( Ntk const& ntk, Simulator const& sim = Simulator() )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_constant_value_v<Ntk>, "Ntk does not implement the constant_value method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_fanin_size_v<Ntk>, "Ntk does not implement the fanin_size method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_compute_v<Ntk, SimulationType>, "Ntk does not implement the compute method for SimulationType" );

  node_map<SimulationType, Ntk> node_to_value( ntk );

  node_to_value[ntk.get_node( ntk.get_constant( false ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( false ) ) ) );
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    node_to_value[ntk.get_node( ntk.get_constant( true ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( true ) ) ) );
  }
  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    node_to_value[n] = sim.compute_pi( i );
  } );

  ntk.foreach_gate( [&]( auto const& n ) {
    std::vector<SimulationType> fanin_values( ntk.fanin_size( n ) );
    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      fanin_values[i] = node_to_value[f];
    } );
    node_to_value[n] = ntk.compute( n, fanin_values.begin(), fanin_values.end() );
  } );

  return node_to_value;
}

/*! \brief Simulates a network with a generic simulator.
 *
 * This is a generic simulation algorithm that can simulate arbitrary values.
 * In order to that, the network needs to implement the `compute` method for
 * `SimulationType` and one must pass an instance of a `Simulator` that
 * implements the three methods:
 * - `SimulationType compute_constant(bool)`
 * - `SimulationType compute_pi(index)`
 * - `SimulationType compute_not(SimulationType const&)`
 *
 * The method `compute_constant` returns a simulation value for a constant
 * value.  The method `compute_pi` returns a simulation value for a primary
 * input based on its index, and `compute_not` to invert a simulation value.
 *
 * This method returns a map that maps each node to its computed simulation
 * value.
 *
 * **Required network functions:**
 * - `foreach_po`
 * - `get_constant`
 * - `constant_value`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_gate`
 * - `fanin_size`
 * - `num_pos`
 * - `compute<SimulationType>`
 *
 * \param ntk Network
 * \param node_to_value A map from nodes to values
 * \param sim Simulator, which implements the simulator interface
 */
template<class SimulationType, class Ntk, class Simulator = default_simulator<SimulationType>>
void simulate_nodes( Ntk const& ntk, unordered_node_map<SimulationType, Ntk>& node_to_value, Simulator const& sim = Simulator() )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_constant_value_v<Ntk>, "Ntk does not implement the constant_value method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_fanin_size_v<Ntk>, "Ntk does not implement the fanin_size method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_compute_v<Ntk, SimulationType>, "Ntk does not implement the compute method for SimulationType" );

  /* constants */
  if ( !node_to_value.has( ntk.get_node( ntk.get_constant( false ) ) ) )
  {
    node_to_value[ntk.get_node( ntk.get_constant( false ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( false ) ) ) );
  }
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    if ( !node_to_value.has( ntk.get_node( ntk.get_constant( true ) ) ) )
    {
      node_to_value[ntk.get_node( ntk.get_constant( true ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( true ) ) ) );
    }
  }

  /* pis */
  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    if ( !node_to_value.has( n ) )
    {
      node_to_value[n] = sim.compute_pi( i );
    }
  } );

  /* gates */
  ntk.foreach_gate( [&]( auto const& n ) {
    if ( !node_to_value.has( n ) )
    {
      std::vector<SimulationType> fanin_values( ntk.fanin_size( n ) );
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
        fanin_values[i] = node_to_value[ntk.get_node( f )];
      } );

      node_to_value[n] = ntk.compute( n, fanin_values.begin(), fanin_values.end() );
    }
  } );
}

/*! \brief Simulates a network with a generic simulator.
 *
 * This is a generic simulation algorithm that can simulate arbitrary values.
 * In order to that, the network needs to implement the `compute` method for
 * `SimulationType` and one must pass an instance of a `Simulator` that
 * implements the three methods:
 * - `SimulationType compute_constant(bool)`
 * - `SimulationType compute_pi(index)`
 * - `SimulationType compute_not(SimulationType const&)`
 *
 * The method `compute_constant` returns a simulation value for a constant
 * value.  The method `compute_pi` returns a simulation value for a primary
 * input based on its index, and `compute_not` to invert a simulation value.
 *
 * This method returns a vector that maps each primary output (ordered by
 * position) to it's simulation value (taking possible complemented attributes
 * into account).
 *
 * **Required network functions:**
 * - `foreach_po`
 * - `is_complemented`
 * - `compute<SimulationType>`
 *
 * \param ntk Network
 * \param sim Simulator, which implements the simulator interface
 */
template<class SimulationType, class Ntk, class Simulator = default_simulator<SimulationType>>
std::vector<SimulationType> simulate( Ntk const& ntk, Simulator const& sim = Simulator() )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po function" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented function" );
  static_assert( has_compute_v<Ntk, SimulationType>, "Ntk does not implement the compute function for SimulationType" );

  const auto node_to_value = simulate_nodes<SimulationType, Ntk, Simulator>( ntk, sim );

  std::vector<SimulationType> po_values( ntk.num_pos() );
  ntk.foreach_po( [&]( auto const& f, auto i ) {
    if ( ntk.is_complemented( f ) )
    {
      po_values[i] = sim.compute_not( node_to_value[f] );
    }
    else
    {
      po_values[i] = node_to_value[f];
    }
  } );
  return po_values;
}

} // namespace mockturtle
