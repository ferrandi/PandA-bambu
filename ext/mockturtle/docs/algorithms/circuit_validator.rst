Functional equivalence of circuit nodes
---------------------------------------

**Header:** ``mockturtle/algorithms/circuit_validator.hpp``

This class can be used to validate potential circuit optimization choices. It checks the functional equivalence of a circuit node with an existing or non-existing signal with SAT, with optional consideration of observability don't-care (ODC).

If more advanced SAT validation is needed, one could consider using ``cnf_view`` instead, which also constructs the CNF clauses of circuit nodes.

**Example**

The following code shows how to check functional equivalence of a root node to signals existing in the network, or created with nodes within the network. If not, get the counter example.

.. code-block:: c++

   /* derive some AIG (can be AIG, XAG, MIG, or XMG) */
   aig_network aig;
   auto const a = aig.create_pi();
   auto const b = aig.create_pi();
   auto const f1 = aig.create_and( !a, b );
   auto const f2 = aig.create_and( a, !b );
   auto const f3 = aig.create_or( f1, f2 );

   circuit_validator v( aig );

   auto result = v.validate( f1, f2 );
   /* result is an optional, which is nullopt if SAT conflict limit was exceeded */
   if ( result )
   {
      if ( *result )
      {
         std::cout << "f1 and f2 are functionally equivalent\n";
      }
      else
      {
         std::cout << "f1 and f2 have different values under PI assignment: ";
         std::cout << v.cex[0] << v.cex[1] << "\n";
      }
   }

   std::vector<aig_network::node> divs;
   divs.emplace_back( aig.get_node( f1 ) );
   divs.emplace_back( aig.get_node( f2 ) );

   xag_index_list id_list;
   id_list.add_inputs( 2 );
   id_list.add_and( 3, 5 ); // f3 = NOT f1 AND NOT f2
   id_list.add_and( 2, 6 ); // f4 = f1 AND f3
   id_list.add_output( 9 ); // NOT f4

   result = v.validate( f3, divs, id_list );
   if ( result && *result )
   {
     std::cout << "f3 is equivalent to NOT(f1 AND (NOT f1 AND NOT f2))\n";
   }

**Parameters**

.. doxygenstruct:: mockturtle::validator_params
   :members:

**Validate with existing signals**

.. doxygenfunction:: mockturtle::circuit_validator::validate( signal const&, signal const& )
.. doxygenfunction:: mockturtle::circuit_validator::validate( node const&, signal const& )
.. doxygenfunction:: mockturtle::circuit_validator::validate( signal const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( node const&, bool )

**Validate with non-existing circuit**

A not-yet-created circuit built on existing nodes in the network can be represented by an index list and a list of support nodes.
The index list must be one of the classes implemented in :ref:`index_list`.

.. doxygenfunction:: mockturtle::circuit_validator::validate( signal const&, std::vector<node> const&, index_list_type const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( node const&, std::vector<node> const&, index_list_type const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( signal const&, iterator_type, iterator_type, index_list_type const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( node const&, iterator_type, iterator_type, index_list_type const&, bool )

**Utilizing don't-cares**

.. doxygenfunction:: mockturtle::circuit_validator::set_odc_levels
.. doxygenfunction:: mockturtle::circuit_validator::update

**Generate multiple patterns**

A simulation pattern is a collection of value assignments to every primary inputs.
A counter-example of a failing validation is a simulation pattern under which the nodes being validated have different simulation values. 
It can be directly read from the public data member ``circuit_validator::cex`` (which is a ``std::vector<bool>`` of size ``Ntk::num_pis()``) after a call to (any type of) ``circuit_validator::validate`` which returns ``false``.
If multiple different patterns are desired, one can call ``circuit_validator::generate_pattern``. However, this is currently only supported for constant validation.

.. doxygenfunction:: mockturtle::circuit_validator::generate_pattern( signal const&, bool, std::vector<std::vector<bool>> const&, uint32_t )
.. doxygenfunction:: mockturtle::circuit_validator::generate_pattern( node const&, bool, std::vector<std::vector<bool>> const&, uint32_t )
