Refactoring
-----------

**Header:** ``mockturtle/algorithms/refactoring.hpp``

The following example shows how to refactor an MIG using the Akers synthesis
method.

.. code-block:: c++

   /* derive some MIG */
   mig_network mig = ...;

   /* node resynthesis */
   akers_resynthesis resyn;
   refactoring( mig, resyn );
   mig = cleanup_dangling( mig );

It is possible to change the cost function of nodes in refactoring.  Here is
an example, in which the cost function does not account for XOR gates in a network. 
This may be helpful in logic synthesis addressing cryptography applications where
XOR gates are considered "for free". 

.. code-block:: c++

   template<class Ntk>
   struct free_xor_cost
   {
     uint32_t operator()( Ntk const& ntk, node<Ntk> const& n ) const
     {
       return ntk.is_xor( n ) ? 0 : 1;
     }
   };

   SomeResynthesisClass resyn;
   refactoring( ntk, resyn, free_xor_cost<Ntk>());

Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::refactoring_params
   :members:

.. doxygenstruct:: mockturtle::refactoring_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::refactoring

Rewriting functions
~~~~~~~~~~~~~~~~~~~

One can use resynthesis functions that can be passed to `node_resynthesis`, see
:ref:`node_resynthesis_functions`.
