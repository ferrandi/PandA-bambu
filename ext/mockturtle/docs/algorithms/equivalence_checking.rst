Equivalence checking
--------------------

**Header:** ``mockturtle/algorithms/equivalence_checking.hpp``


.. code-block:: c++

   /* derive some AIG and make a copy */
   aig_network aig = ...;
   const auto orig = aig;

   /* node resynthesis */
   xag_npn_resynthesis<aig_network> resyn;
   cut_rewriting_params ps;
   ps.cut_enumeration_ps.cut_size = 4;
   aig = cut_rewriting( aig, resyn, ps );

   const auto miter = *miter<aig_network>( orig, aig );
   const auto result = equivalence_checking( miter );

   /* result is an optional, which is nullopt if no solution was found */
   if ( result && *result )
   {
     std::cout << "networks are equivalent\n";
   }

Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::equivalence_checking_params
   :members:

.. doxygenstruct:: mockturtle::equivalence_checking_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::equivalence_checking
