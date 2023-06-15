.. _cost_generic_resub:

Cost-generic resubstitution algorithm
-------------------------------------

**Header:** ``mockturtle/algorithms/experimental/cost_generic_resub.hpp``

This header file defines a resubstitution algorithm to optimize a network with a customized cost function. 

.. code-block:: c++

   /* derive some XAG */
   xag_network xag = ...;

   cost_generic_resub_params ps;
   cost_generic_resub_stats st;

   cost_generic_resub( xag, xag_size_cost_function<xag_network>(), ps, &st );
   xag = cleanup_dangling( xag );

Customized cost function
~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::recursive_cost_functions
   :members:
   :no-link:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::experimental::cost_generic_resub
