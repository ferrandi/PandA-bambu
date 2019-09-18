DSD decomposition
-----------------

Here is an example how DSD decomposition can be used to create a *k*-LUT network
from a truth table.

.. code-block:: c++

   /* input function */
   kitty::dynamic_truth_table table( 5u );
   kitty::create_from_expression( table, "{a<(bc)de>}" );

   klut_network ntk;
   const auto x1 = ntk.create_pi();
   const auto x2 = ntk.create_pi();
   const auto x3 = ntk.create_pi();
   const auto x4 = ntk.create_pi();
   const auto x5 = ntk.create_pi();

   auto fn = [&]( kitty::dynamic_truth_table const& remainder, std::vector<klut_network::signal> const& children ) {
     return ntk.create_node( children, remainder );
   };

   ntk.create_po( dsd_decomposition( ntk, table, {x1, x2, x3, x4, x5}, fn ) );

   write_bench( ntk, std::cout );

The output is::

   INPUT(n2)
   INPUT(n3)
   INPUT(n4)
   INPUT(n5)
   INPUT(n6)
   OUTPUT(po0)
   n0 = gnd
   n1 = vdd
   n7 = LUT 0x8 (n3, n4)
   n8 = LUT 0xe8 (n7, n5, n6)
   n9 = LUT 0xe (n2, n8)
   po0 = LUT 0x2 (n9)

That is DSD decomposition extracted the OR gate on the top and the AND gate
on the bottom. The remaining majority-function is non-decomposable and is
constructed as a node using the callback function.

**Header:** ``mockturtle/algorithms/dsd_decomposition.hpp``

.. doxygenfunction:: mockturtle::dsd_decomposition
