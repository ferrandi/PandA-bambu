Collapse mapped network
-----------------------

**Header:** ``mockturtle/algorithms/collapse_mapped.hpp``

The following example shows how to collapse a mapped network into a
:math:`k`-LUT network:

.. code-block:: c++

   aig_network aig = ...;
   mapping_view<aig_network, true> mapped_aig{aig};
   lut_mapping<<aig_network, true>, true>( mapped_aig );
   const auto klut = collapse_mapped_network<klut_network>( mapped_aig );
   write_bench( klut, "/tmp/test.bench" );

.. doxygenfunction:: mockturtle::collapse_mapped_network(NtkDest&, NtkSource const&)
.. doxygenfunction:: mockturtle::collapse_mapped_network(NtkSource const&)

