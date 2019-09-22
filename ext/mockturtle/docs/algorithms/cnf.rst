CNF generation
--------------

**Header:** ``mockturtle/algorithms/cnf.hpp``

The following code shows how to create clauses for a SAT solver based on a
network.

.. code-block:: c++

   /* derive some XAG (can be any network type) */
   xag_network xag = ...;

   percy::bsat_wrapper solver;
   const auto output_lits = generate_cnf( xag, [&]( auto const& clause ) {
     solver.add_clause( clause );
   } );

.. doxygenfunction:: mockturtle::node_literals
.. doxygenfunction:: mockturtle::generate_cnf
.. doxygentypedef:: mockturtle::clause_callback_t
