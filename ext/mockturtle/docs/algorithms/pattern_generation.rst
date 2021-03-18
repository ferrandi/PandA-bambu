Simulation pattern generation
-----------------------------

**Header:** ``mockturtle/algorithms/pattern_generation.hpp``

The following example shows how to generate expressive simulation patterns
for an AIG network. It starts with 256 random patterns, generates stuck-at
patterns to make every node's signature has at least 3 bits of 0 and 3 bits
of 1 which are observable at at least 5 levels of fan-out. The generated
patterns are then written out to a file.

.. code-block:: c++

   /* derive some AIG */
   aig_network aig = ...;

   pattern_generation_params ps;
   ps.num_stuck_at = 3;
   ps.odc_levels = 5;

   partial_simulator sim( aig.num_pis(), 256 );
   pattern_generation( aig, sim, ps );
   write_patterns( sim, "patterns.pat" );


Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::pattern_generation_params
   :members:

.. doxygenstruct:: mockturtle::pattern_generation_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::pattern_generation
