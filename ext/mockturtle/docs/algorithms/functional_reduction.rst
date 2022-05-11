Functional reduction
--------------------

**Header:** ``mockturtle/algorithms/functional_reduction.hpp``

The following example shows how to perform functional reduction
to remove constant nodes and functionally equivalent nodes in
the network.

.. code-block:: c++

   /* derive some AIG */
   aig_network aig = ...;

   functional_reduction( aig );


Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::functional_reduction_params
   :members:

.. doxygenstruct:: mockturtle::functional_reduction_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::functional_reduction
