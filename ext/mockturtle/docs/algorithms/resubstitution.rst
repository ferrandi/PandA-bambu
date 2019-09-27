Resubstitution
--------------

**Header:** ``mockturtle/algorithms/resubstitution.hpp``

The following example shows how to resubstitute nodes in an MIG.

.. code-block:: c++

   /* derive some MIG */
   mig_network mig = ...;

   /* resubstitute nodes */
   resubstitution( mig );
   mig = cleanup_dangling( mig );

Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::resubstitution_params
   :members:

.. doxygenstruct:: mockturtle::resubstitution_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::resubstitution
