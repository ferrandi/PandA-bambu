.. _buffer_insertion:

AQFP buffer insertion and verification
--------------------------------------

**Header:** ``mockturtle/algorithms/aqfp/buffer_insertion.hpp``

Technology assumptions
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::aqfp_assumptions
   :members:

Buffer insertion algorithms
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::buffer_insertion
   :members:

Parameters
~~~~~~~~~~

.. doxygenstruct:: mockturtle::buffer_insertion_params
   :members:

Buffered network data structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The results of this algorithm can be dumped into an appropriate :ref:buffered_network type
and written out in the Verilog format.


Verification of buffered networks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/algorithms/aqfp/buffer_verification.hpp``

.. doxygenfunction:: mockturtle::verify_aqfp_buffer( Ntk const&, aqfp_assumptions const& )

.. doxygenfunction:: mockturtle::schedule_buffered_network( Ntk const& ntk, aqfp_assumptions const& ps )

.. doxygenfunction:: mockturtle::verify_aqfp_buffer( Ntk const&, aqfp_assumptions const&, node_map<uint32_t, Ntk> const& )

