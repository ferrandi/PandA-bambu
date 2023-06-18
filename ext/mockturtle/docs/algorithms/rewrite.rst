Rewrite
-------

**Header:** ``mockturtle/algorithms/rewrite.hpp``

The following example shows how to rewrite an MIG using precomputed optimum
networks.  In this case the maximum number of variables for a node function is
4.

.. code-block:: c++

   /* derive some MIG */
   mig_network mig = ...;

   /* node resynthesis */
   mig_npn_resynthesis resyn;
   exact_library_params eps;
   eps.np_classification = false;
   exact_library<xag_network, decltype( resyn )> exact_lib( resyn, eps );

   /* rewrite */
   rewrite( mig, exact_lib );

It is possible to change the cost function of nodes in rewrite.  Here is
an example, in which the cost function only accounts for AND gates in a network,
which corresponds to the multiplicative complexity of a function.

.. code-block:: c++

   template<class Ntk>
   struct mc_cost
   {
     uint32_t operator()( Ntk const& ntk, node<Ntk> const& n ) const
     {
       return ntk.is_and( n ) ? 1 : 0;
     }
   };

   SomeResynthesisClass resyn;
   exact_library_params eps;
   eps.np_classification = false;
   exact_library<xag_network, decltype( resyn )> exact_lib( resyn, eps );
   rewrite<decltype( Ntk ), decltype( exact_lib ), mc_cost>( ntk, exact_lib );

Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::rewrite_params
   :members:

.. doxygenstruct:: mockturtle::rewrite_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::rewrite

Rewriting functions
~~~~~~~~~~~~~~~~~~~

One can use resynthesis functions with a pre-computed database and process
them using :ref:`exact_library`, see :ref:`node_resynthesis_functions`.
