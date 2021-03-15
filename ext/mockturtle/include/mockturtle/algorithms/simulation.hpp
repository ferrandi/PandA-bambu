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
#include <fstream>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../networks/aig.hpp"
#include "../networks/xag.hpp"

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/partial_truth_table.hpp>
#include <kitty/print.hpp>
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
template<uint32_t NumVars>
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

/*! \brief Simulates partial truth tables.
 *
 * This simulator simulates partial truth tables, whose length is flexible
 * and new simulation patterns can be added.
 */
class partial_simulator
{
public:
  partial_simulator() = delete;

  /*! \brief Create a `partial_simulator` with random simulation patterns.
   *
   * \param num_pis Number of primary inputs, which is the same as the length of a simulation pattern.
   * \param num_patterns Number of initial random simulation patterns.
   */
  partial_simulator( unsigned num_pis, unsigned num_patterns, std::default_random_engine::result_type seed = 0 )
    : num_patterns( num_patterns )
  {
    assert( num_pis > 0u );

    for ( auto i = 0u; i < num_pis; ++i )
    {
      patterns.emplace_back( num_patterns );
      kitty::create_random( patterns.back(), seed + i );
    }
  }

  /* copy constructor */
  partial_simulator( partial_simulator const& sim )
    : patterns( sim.patterns ), num_patterns( sim.num_patterns )
  { }

  /*! \brief Create a `partial_simulator` with given simulation patterns.
   *
   * \param initial_patterns Initial simulation patterns.
   */
  partial_simulator( std::vector<kitty::partial_truth_table> const& initial_patterns )
    : patterns( initial_patterns ), num_patterns( patterns.at( 0 ).num_bits() )
  { }

  /*! \brief Create a `partial_simulator` with simulation patterns read from a file.
   *
   * The simulation pattern file should contain `num_pis` lines of the same length.
   * Each line is the simulation signature of a primary input, represented in hexadecimal.
   *
   * \param fielname Name of the simulation pattern file.
   * \param length Number of simulation patterns to keep. Should not be greater than 4 times 
   * the length of a line in the file. Setting this parameter to 0 means to keep all patterns in the file.
   */
  partial_simulator( const std::string& filename, uint32_t length = 0u )
  {
    std::ifstream in( filename, std::ifstream::in );
    std::string line;

    while ( getline( in, line ) )
    {
      patterns.emplace_back( line.length() * 4 );
      kitty::create_from_hex_string( patterns.back(), line );
      if ( length != 0u )
      {
        patterns.back().resize( length );
      }
    }

    in.close();

    assert( patterns.size() > 0 );
    num_patterns = patterns[0].num_bits();
  }

  kitty::partial_truth_table compute_constant( bool value ) const
  {
    kitty::partial_truth_table zero( num_patterns );
    return value ? ~zero : zero;
  }

  kitty::partial_truth_table compute_pi( uint32_t index ) const
  {
    return patterns.at( index );
  }

  kitty::partial_truth_table compute_not( kitty::partial_truth_table const& value ) const
  {
    return ~value;
  }

  /*! \brief Get the current number of simulation patterns. */
  uint32_t num_bits() const
  {
    return num_patterns;
  }

  void add_pattern( std::vector<bool> const& pattern )
  {
    assert( pattern.size() == patterns.size() );

    for ( auto i = 0u; i < pattern.size(); ++i )
    {
      patterns.at( i ).add_bit( pattern.at( i ) );
    }
    ++num_patterns;
  }

  /*! \brief Writes the simulation patterns into a file. */
  void write_patterns( const std::string& filename )
  {
    std::ofstream out( filename, std::ofstream::out );
    for ( auto i = 0u; i < patterns.size(); ++i )
    {
      out << kitty::to_hex( patterns.at( i ) ) << "\n";
    }
    out.close();
  }

private:
  std::vector<kitty::partial_truth_table> patterns;
  uint32_t num_patterns;
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

namespace detail
{

template<class Ntk>
void simulate_fanin_cone( Ntk const& ntk, typename Ntk::node const& n, unordered_node_map<kitty::partial_truth_table, Ntk>& node_to_value, partial_simulator const& sim )
{
  std::vector<kitty::partial_truth_table> fanin_values( ntk.fanin_size( n ) );
  ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
    if ( !node_to_value.has( ntk.get_node( f ) ) )
    {
      simulate_fanin_cone( ntk, ntk.get_node( f ), node_to_value, sim );
    }
    else assert( node_to_value[ntk.get_node( f )].num_bits() == sim.num_bits() );
    fanin_values[i] = node_to_value[ntk.get_node( f )];
  } );
  node_to_value[n] = ntk.compute( n, fanin_values.begin(), fanin_values.end() );
}

template<class Ntk>
void re_simulate_fanin_cone( Ntk const& ntk, typename Ntk::node const& n, unordered_node_map<kitty::partial_truth_table, Ntk>& node_to_value, partial_simulator const& sim )
{
  std::vector<kitty::partial_truth_table> fanin_values( ntk.fanin_size( n ) );
  ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
    assert( node_to_value.has( ntk.get_node( f ) ) );
    if ( node_to_value[ntk.get_node( f )].num_bits() != sim.num_bits() )
    {
      re_simulate_fanin_cone( ntk, ntk.get_node( f ), node_to_value, sim );
    }
    fanin_values[i] = node_to_value[ntk.get_node( f )];
  } );
  ntk.compute( n, node_to_value[n], fanin_values.begin(), fanin_values.end() );
}

template<class Ntk>
void update_const_pi( Ntk const& ntk, unordered_node_map<kitty::partial_truth_table, Ntk>& node_to_value, partial_simulator const& sim )
{
  /* constants */
  node_to_value[ntk.get_node( ntk.get_constant( false ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( false ) ) ) );
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    node_to_value[ntk.get_node( ntk.get_constant( true ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( true ) ) ) );
  }

  /* pis */
  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    node_to_value[n] = sim.compute_pi( i );
  } );
}

} // namespace detail

/*! \brief (re-)simulate `n` and its transitive fanin cone.
 * 
 * It is assumed that `node_to_value.has( n )` is true for every node except `n`,
 * but not necessarily up to date. If not, needed nodes in its transitive fanin cone will be re-simulated.
 * Note that re-simulation is only done for the last block, no matter how many bits are used in this block.
 * Hence, it is advised to call `simulate_nodes` with `simulate_whole_tt = false` whenever `sim.num_bits() % 64 == 0`.
 * 
 */
template<class Ntk>
void simulate_node( Ntk const& ntk, typename Ntk::node const& n, unordered_node_map<kitty::partial_truth_table, Ntk>& node_to_value, partial_simulator const& sim )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_constant_value_v<Ntk>, "Ntk does not implement the constant_value method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_compute_v<Ntk, kitty::partial_truth_table>, "Ntk does not implement the compute method for kitty::partial_truth_table" );
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network> || std::is_same_v<typename Ntk::base_type, xag_network>, "The partial_truth_table specialized ntk.compute is currently only implemented in AIG and XAG" );

  if ( !node_to_value.has( n ) )
  {
    std::vector<kitty::partial_truth_table> fanin_values( ntk.fanin_size( n ) );
    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      assert( node_to_value.has( ntk.get_node( f ) ) );
      if ( node_to_value[ntk.get_node( f )].num_bits() != sim.num_bits() )
      {
        detail::re_simulate_fanin_cone( ntk, ntk.get_node( f ), node_to_value, sim );
      }
      fanin_values[i] = node_to_value[ntk.get_node( f )];
    } );
    node_to_value[n] = ntk.compute( n, fanin_values.begin(), fanin_values.end() );
  }
  else if ( node_to_value[n].num_bits() != sim.num_bits() )
  {
    if ( node_to_value[ntk.get_node( ntk.get_constant( false ) )].num_bits() != sim.num_bits() )
    {
      detail::update_const_pi( ntk, node_to_value, sim );
    }
    detail::re_simulate_fanin_cone( ntk, n, node_to_value, sim );
  }
}

/*! \brief Simulates a network with a partial simulator.
 *
 * This is the specialization for `partial_truth_table`.
 * This function simulates every node in the circuit.
 *
 * \param simulate_whole_tt When this parameter is true, it is assumed that `node_to_value.has( n )` is false for every node.
 * In contrast, when this parameter is false, only the last block of `partial_truth_table` will be re-computed,
 * and it is assumed that `node_to_value.has( n )` is true for every node.
 */
template<class Ntk>
void simulate_nodes( Ntk const& ntk, unordered_node_map<kitty::partial_truth_table, Ntk>& node_to_value, partial_simulator const& sim, bool simulate_whole_tt = true )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_constant_value_v<Ntk>, "Ntk does not implement the constant_value method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_compute_v<Ntk, kitty::partial_truth_table>, "Ntk does not implement the compute method for kitty::partial_truth_table" );
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network> || std::is_same_v<typename Ntk::base_type, xag_network>, "The partial_truth_table specialized ntk.compute is currently only implemented in AIG and XAG" );

  detail::update_const_pi( ntk, node_to_value, sim );

  /* gates */
  if ( simulate_whole_tt )
  {
    ntk.foreach_gate( [&]( auto const& n ) {
      if ( !node_to_value.has( n ) )
      {
        detail::simulate_fanin_cone( ntk, n, node_to_value, sim );
      }
    } );
  }
  else
  {
    ntk.foreach_gate( [&]( auto const& n ) {
      assert( node_to_value.has( n ) );
      if ( node_to_value[n].num_bits() != sim.num_bits() )
      {
        detail::re_simulate_fanin_cone( ntk, n, node_to_value, sim );
      }
    } );
  }
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
