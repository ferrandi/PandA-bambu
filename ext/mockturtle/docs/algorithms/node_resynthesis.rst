Node resynthesis
----------------

**Header:** ``mockturtle/algorithms/node_resynthesis.hpp``

The following example shows how to resynthesize a `k`-LUT network derived from
an AIG using LUT mapping into an MIG using precomputed optimum networks.  In
this case the maximum number of variables for a node function is 4.

.. code-block:: c++

   /* derive some AIG */
   aig_network aig = ...;

   /* LUT mapping */
   mapping_view<aig_network, true> mapped_aig{aig};
   lut_mapping_params ps;
   ps.cut_enumeration_ps.cut_size = 4;
   lut_mapping<mapping_view<aig_network, true>, true>( mapped_aig, ps );

   /* collapse into k-LUT network */
   const auto klut = *collapse_mapped_network<klut_network>( mapped_aig );

   /* node resynthesis */
   mig_npn_resynthesis resyn;
   const auto mig = node_resynthesis<mig_network>( klut, resyn );

Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::node_resynthesis_params
   :members:

.. doxygenstruct:: mockturtle::node_resynthesis_stats
   :members:

Algorithm
~~~~~~~~~

.. doxygenfunction:: mockturtle::node_resynthesis

.. _node_resynthesis_functions:

Resynthesis functions
~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::xag_npn_resynthesis

.. doxygenclass:: mockturtle::mig_npn_resynthesis

.. doxygenclass:: mockturtle::xmg_npn_resynthesis

.. doxygenclass:: mockturtle::xag_minmc_resynthesis
   :members:

.. doxygenclass:: mockturtle::exact_resynthesis

.. doxygenclass:: mockturtle::exact_aig_resynthesis

.. doxygenclass:: mockturtle::dsd_resynthesis

.. doxygenclass:: mockturtle::shannon_resynthesis

.. doxygenclass:: mockturtle::positive_davio_resynthesis

.. doxygenclass:: mockturtle::negative_davio_resynthesis

.. doxygenclass:: mockturtle::direct_resynthesis

.. doxygenclass:: mockturtle::akers_resynthesis

.. doxygenclass:: mockturtle::bidecomposition_resynthesis