Akers synthesis
---------------

**Header:** ``mockturtle/algorithms/akers_synthesis.hpp``

The algorithm implements the method for 3-input majority-based logic synthesis proposed in
“*Synthesis of combinational logic using three-input majority
gates*” by Sheldon B. Akers, Jr. (1962).

This method is implemented in the function `akers_synthesis`, which takes as
input a function represented as truth table with possible don't cares.

The easiest way to run Akers' synthesis is by doing:

.. code-block:: c++

   auto mig = akers_synthesis<mig_network>( func, care );

Here, the function returns an MIG, with as many inputs as the number of
variables of func and one primary output.

The synthesis method can also be run using already existing signals from an MIG as inputs.
As an example, consider an MIG with 4 primary inputs `a`, `b`, `c`, and `d`,
as well as two gates:

.. code-block:: c++

   std::vector<mig_network::signal> gates;
   gates.push_back( mig.create_and( a, b ) );
   gates.push_back( mig.create_and( c, d ) );

   auto t = akers_synthesis( mig, func, care, gates.begin(), gates.end() );

The algorithm performs the synthesis of the function considering as inputs the two AND gates.

.. doxygenfunction:: mockturtle::akers_synthesis(Ntk&, kitty::dynamic_truth_table const&, kitty::dynamic_truth_table const&, LeavesIterator, LeavesIterator)

.. doxygenfunction:: mockturtle::akers_synthesis(kitty::dynamic_truth_table const&, kitty::dynamic_truth_table const&)
