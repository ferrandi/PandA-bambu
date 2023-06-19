Cut enumeration
---------------

**Header:** ``mockturtle/algorithms/cut_enumeration.hpp``

A *cut* of a Boolean network is a pair :math:`(n,L)`, where :math:`n` is a
node, called *root*, and :math:`L` is a set of nodes, called *leaves*, such
that (i) each path from any primary input to :math:`n` passes through at least
one leaf and (ii) for each leaf :math:`l \in L`, there is at least one path
from a primary input to :math:`n` passing through :math:`l` and not through any
other leaf.

The function :cpp:func:`mockturtle::cut_enumeration` implements cut
enumeration, which is an algorithm that computes all cuts of all nodes in a
Boolean network.  Since the number of cuts is very large, the number of
enumerated cuts is bounded by a parameter :math:`l` (`cut_size`) for the cut
size and a parameter :math:`p` (`cut_limit`) for the maximum number of cuts for
each node.  This technique is referred to as priority cuts as it selects the
subset of all cuts with respect to some cost function, in our case the number
of the cuts' leaves.  The returned cut sets are also irredundant and do not
contain two cuts :math:`(n, L_1)` and :math:`(n, L_2)` such that :math:`L_1`
dominates :math:`L_2`, i.e., :math:`L_1 \subseteq L_2`.

The simplest way to enumerate cuts in some network ``ntk`` is by calling:

.. code-block:: c++

   auto cuts = cut_enumeration( ntk );

   ntk.foreach_node( [&]( auto node ) {
     std::cout << cuts.cuts( ntk.node_to_index( node ) ) << "\n";
   } );

Parameters can be set to adjust the cut size and the number of maximum cuts
for each node:

.. code-block:: c++

   cut_enumeration_params ps;
   ps.cut_size = 6;
   ps.cut_limit = 8;

   auto cuts = cut_enumeration( ntk, ps );

A template argument to `cut_enumeration` can enable truth table computation for
each cut.  The truth table for each cut can be retrieved from the return value
for `cut_enumeration`.  The following example enumerates all cuts and computes
their truth tables.  Afterwards, the truth table for each cut is printed.  In
the example, `Ntk` is the type of `ntk`.

.. code-block:: c++

   auto cuts = cut_enumeration<Ntk, true>( ntk ); /* true enables truth table computation */
   ntk.foreach_node( [&]( auto n ) {
     const auto index = ntk.node_to_index( n );
     for ( auto const& cut : cuts.cuts( index ) )
     {
       std::cout << "Cut " << *cut
                 << " with truth table " << kitty::to_hex( cuts.truth_table( *cut ) )
                 << "\n";
     }
   } );

Parameters
~~~~~~~~~~

.. doxygenstruct:: mockturtle::cut_enumeration_params
   :members:

Return value
~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::network_cuts
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::cut_enumeration

Pre-defined cut types
~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/algorithms/cut_enumeration/gia_cut.hpp``

.. doxygenstruct:: mockturtle::cut_enumeration_gia_cut

**Header:** ``mockturtle/algorithms/cut_enumeration/mf_cut.hpp``

.. doxygenstruct:: mockturtle::cut_enumeration_mf_cut

**Header:** ``mockturtle/algorithms/cut_enumeration/cnf_cut.hpp``

.. doxygenstruct:: mockturtle::cut_enumeration_cnf_cut

**Header:** ``mockturtle/algorithms/cut_enumeration/spectr_cut.hpp``

.. doxygenstruct:: mockturtle::cut_enumeration_spectr_cut

**Header:** ``mockturtle/algorithms/cut_enumeration/tech_map_cut.hpp``

.. doxygenstruct:: mockturtle::cut_enumeration_tech_map_cut

**Header:** ``mockturtle/algorithms/cut_enumeration/exact_map_cut.hpp``

.. doxygenstruct:: mockturtle::cut_enumeration_exact_map_cut

Special-purpose implementations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. doxygenfunction:: mockturtle::fast_cut_enumeration

.. doxygenfunction:: mockturtle::fast_small_cut_enumeration
