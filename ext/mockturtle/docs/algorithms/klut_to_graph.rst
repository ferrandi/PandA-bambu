k-LUT to graph conversion
-------------------------

**Header:** ``mockturtle/algorithms/klut_to_graph.hpp``

This header file implements utility functions to convert a :math:`k`-LUT network into a
homogeneous graph network, such as AIG, XAG, MIG or XMG. It wraps around three node resynthesis strategies for the user.

First the function attempts a Disjoint Support Decomposition (DSD), branching the network into subnetworks. 
As soon as DSD can no longer be done, there are two possibilities depending on the dimensionality of the subnetwork to be resynthesized.
On the one hand, if the size of the associated support is lower or equal than 4, the solution can be recovered by exploiting the mapping of the subnetwork to its NPN-class. 
On the other hand, if the support size is higher than 4, A Shannon decomposition is performed, branching the network in further subnetworks with reduced support.
Finally, once the threshold value of 4 is reached, the NPN mapping completes the graph definition.

There is an out-of-place version, and an in-place version of the function.

The following example shows how to convert a :math:`k`-LUT network into an AIG, a XAG, a MIG and a XMG.

.. code-block:: c++

   /* derive some k-LUT */
   const klut_network klut = ...;

   /* out-of-place version */
   aig_network aig = convert_klut_to_graph<aig_network>( klut );
   xag_network xag = convert_klut_to_graph<xag_network>( klut );

   /* in-place version */
   mig_network mig;
   xmg_network xmg;
   convert_klut_to_graph<mig_network>( mig, klut );
   convert_klut_to_graph<xmg_network>( xmg, klut );

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::convert_klut_to_graph( NtkSrc const& )

.. doxygenfunction:: mockturtle::convert_klut_to_graph( NtkDest&, NtkSrc const& )

