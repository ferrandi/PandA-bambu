Network simulation
------------------

**Header:** ``mockturtle/algorithms/simulation.hpp``

**Examples**

Simulate a Boolean input assignment.

.. code-block:: c++

   aig_network aig = ...;

   std::vector<bool> assignment( aig.num_pis() );
   assignment[0] = true;
   assignment[1] = false;
   assignment[2] = false;
   ...

   default_simulator<bool> sim( assignment );
   const auto values = simulate<bool>( aig, sim );

   aig.foreach_po( [&]( auto const&, auto i ) {
     std::cout << fmt::format( "output {} has simulation value {}\n", i, values[i] );
   } );

Complete simulation with truth tables.

.. code-block:: c++

   aig_network aig = ...;

   default_simulator<kitty::dynamic_truth_table> sim( aig.num_pis() );
   const auto tts = simulate<kitty::dynamic_truth_table>( aig, sim );

   aig.foreach_po( [&]( auto const&, auto i ) {
     std::cout << fmt::format( "truth table of output {} is {}\n", i, kitty::to_hex( tts[i] ) );
   } );

Simulate values for all nodes.

.. code-block:: c++

   aig_network aig = ...;

   default_simulator<kitty::dynamic_truth_table> sim( aig.num_pis() );
   const auto tts = simulate_nodes<kitty::dynamic_truth_table>( aig, sim );

   aig.foreach_node( [&]( auto const& n, auto i ) {
     std::cout << fmt::format( "truth table of node {} is {}\n", i, tts[n] );
   } );

.. doxygenfunction:: mockturtle::simulate

.. doxygenfunction:: mockturtle::simulate_nodes(Ntk const&, Simulator const&)

.. doxygenfunction:: mockturtle::simulate_nodes(Ntk const&, unordered_node_map<SimulationType, Ntk>&, Simulator const&)

Simulators
~~~~~~~~~~

The following simulators are implemented:

* ``mockturtle::default_simulator<bool>``: This simulator simulates Boolean
  values.  A vector with assignments for each primary input must be passed to
  the constructor.
* ``mockturtle::default_simulator<kitty::static_truth_table<NumVars>>``: This
  simulator simulates truth tables.  Each primary input is assigned the
  projection function according to the index.  The number of variables must be
  known at compile time.
* ``mockturtle::default_simulator<kitty::dynamic_truth_table>``: This simulator
  simulates truth tables.  Each primary input is assigned the projection
  function according to the index.  The number of variables be passed to the
  constructor of the simulator.
* ``mockturtle::partial_simulator``: This simulator simulates partial truth tables,
  whose length is flexible and new simulation patterns can be added.

Partial simulation
~~~~~~~~~~~~~~~~~~

With ``partial_simulator``, adding new simulation patterns and re-simulation can be done.
Incremental simulation is adopted, which speeds up simulation time by only re-simulating needed nodes and only re-computing the last block of ``partial_truth_table``.
Note that currently only AIG and XAG are supported.

.. doxygenfunction:: mockturtle::partial_simulator::partial_simulator( unsigned, unsigned, std::default_random_engine::result_type )

.. doxygenfunction:: mockturtle::partial_simulator::partial_simulator( std::vector<kitty::partial_truth_table> const& )

.. doxygenfunction:: mockturtle::partial_simulator::partial_simulator( const std::string&, uint32_t )

.. doxygenfunction:: mockturtle::partial_simulator::num_bits()

.. doxygenfunction:: mockturtle::partial_simulator::write_patterns( const std::string& )

.. doxygenfunction:: mockturtle::simulate_nodes( Ntk const&, unordered_node_map<kitty::partial_truth_table, Ntk>&, partial_simulator const&, bool )

.. doxygenfunction:: mockturtle::simulate_node( Ntk const&, typename Ntk::node const&, unordered_node_map<kitty::partial_truth_table, Ntk>&, partial_simulator const& )
