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

   circuit_validator<aig_network>::gate::fanin gi1{0, true};
   circuit_validator<aig_network>::gate::fanin gi2{1, true};
   circuit_validator<aig_network>::gate g{{gi1, gi2}, circuit_validator<aig_network>::gate_type::AND};

   circuit_validator<aig_network>::gate::fanin hi1{2, false};
   circuit_validator<aig_network>::gate::fanin hi2{0, false};
   circuit_validator<aig_network>::gate h{{hi1, hi2}, circuit_validator<aig_network>::gate_type::AND};

   result = v.validate( f3, {aig.get_node( f1 ), aig.get_node( f2 )}, {g, h}, true );
   if ( result && *result )
   {
     std::cout << "f3 is equivalent to NOT((NOT f1 AND NOT f2) AND f1)\n";
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

.. doxygenfunction:: mockturtle::circuit_validator::validate( signal const&, std::vector<node> const&, std::vector<gate> const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( node const&, std::vector<node> const&, std::vector<gate> const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( signal const&, iterator_type, iterator_type, std::vector<gate> const&, bool )
.. doxygenfunction:: mockturtle::circuit_validator::validate( node const&, iterator_type, iterator_type, std::vector<gate> const&, bool )

.. doxygenstruct:: mockturtle::circuit_validator::gate
   :members: fanins, type
.. doxygenstruct:: mockturtle::circuit_validator::gate::fanin
   :members: index, inverted

**Updating**

.. doxygenfunction:: mockturtle::circuit_validator::update

**Generate multiple patterns**

A simulation pattern is a collection of value assignments to every primary inputs.
A counter-example of a failing validation is a simulation pattern under which the nodes being validated have different simulation values. 
It can be directly read from the public data member ``circuit_validator::cex`` (which is a ``std::vector<bool>`` of size ``Ntk::num_pis()``) after a call to (any type of) ``circuit_validator::validate`` which returns ``false``.
If multiple different patterns are desired, one can call ``circuit_validator::generate_pattern``. However, this is currently only supported for constant validation.

.. doxygenfunction:: mockturtle::circuit_validator::generate_pattern( signal const&, bool, std::vector<std::vector<bool>> const&, uint32_t )
.. doxygenfunction:: mockturtle::circuit_validator::generate_pattern( node const&, bool, std::vector<std::vector<bool>> const&, uint32_t )
