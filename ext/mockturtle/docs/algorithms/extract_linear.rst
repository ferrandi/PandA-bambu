Extract linear subcircuit
-------------------------

**Header:** ``mockturtle/algorithms/extract_linear.hpp``

These functions can extract the linear part of an XAG.  All AND gates will be
removed.  The two fan-ins of each AND gate will become primary outputs of the
new circuit and the fan-out of the AND gate will be a primary inputs.

.. code-block:: c++

   xag_network xag = ...

   // signals is a vector of three tuples with PI/PO for each AND gate
   const auto [linxag, signals] = extract_linear_circuit( xag );

   // linxag has only XOR gates and inverters possibly only at the primary
   // outputs;
   // xag2 will be equivalent to xag
   const auto xag2 = merge_linear_circuit( linxag, signals.size() );

.. doxygenfunction:: mockturtle::extract_linear_circuit
.. doxygenfunction:: mockturtle::merge_linear_circuit
