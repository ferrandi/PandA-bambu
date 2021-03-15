Resynthesize linear circuit
---------------------------

**Header:** ``mockturtle/algorithms/linear_resynthesis.hpp``

These functions resynthesize a linear circuit, i.e., a circuit that only
consists of XOR gates.  Currently, only XAG-like networks are supported, which
include `xag_network`, but also views on `xag_network`.  The underlying idea of
the algorithms is to first express the input/output behavior in terms of a
Boolean matrix, which is then resynthesized into a new XOR network.

The following example shows how to extract a linear subcircuit from an XAG,
resynthesize it and then merge back the AND gates.

.. code-block:: c++

   xag_network xag = ...

   const auto [linxag, signals] = extract_linear_circuit( xag );
   linxag = linear_resynthesis_paar( linxag );
   xag = merge_linear_circuit( linxag, signals.size() );

.. doxygenfunction:: mockturtle::linear_resynthesis_paar
.. doxygenfunction:: mockturtle::exact_linear_resynthesis
.. doxygenfunction:: mockturtle::get_linear_matrix
.. doxygenfunction:: mockturtle::exact_linear_synthesis
