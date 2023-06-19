.. _sequential:

Sequential networks
===================

**Header:** ``mockturtle/networks/sequential.hpp``

Plain network implementations model the combinational part of circuits.
Representation of memory elements (called registers, though they can also
be used to model latches) can be extended to some network implementations
by wrapping the network with ``sequential``.

.. code-block:: c++

   sequential<aig_network> sequential_aig;
   sequential<xag_network> sequential_xag;
   sequential<mig_network> sequential_mig;
   sequential<xmg_network> sequential_xmg;
   sequential<klut_network> sequential_klut;
   sequential<cover_network> sequential_cover;


Storing register information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::register_t
   :members:

Sequential interface APIs
~~~~~~~~~~~~~~~~~~~~~~~~~

By wrapping a network with ``sequential``, the following interface APIs
are provided (or overwritten).

.. doxygenclass:: mockturtle::sequential
   :members: create_ro, create_ri, is_ci, is_ro, is_combinational, num_cis, num_cos, num_registers, ci_at, co_at, ro_at, ri_at, ci_index, co_index, ri_index, ro_index, ri_to_ro, ro_to_ri, foreach_ci, foreach_co, foreach_ri, foreach_ro, foreach_register, set_register, register_at

