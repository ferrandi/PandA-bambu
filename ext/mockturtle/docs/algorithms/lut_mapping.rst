LUT mapping 1
-------------

Dynamic-programming based heuristic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/algorithms/lut_mapper.hpp``

LUT mapping with cut size :math:`k` partitions a logic network into mapped
nodes and unmapped nodes, where a mapped node :math:`n` is assigned some cut
:math:`(n, L)` such that the following conditions hold: i) each node that
drives a primary output is mapped, and ii) each leaf in the cut of a mapped
node is either a primary input or also mapped.  This ensures that the mapping
covers the whole logic network.  LUT mapping aims to find a good mapping with
respect to a cost function, e.g., a short critical path in the mapping or a
small number of mapped nodes.  The LUT mapping algorithm can assign a weight
to a cut for alternative cost functions.

This implementation offers delay- or area-oriented mapping. The mapper can be
configured using many options.

The following example shows how to perform a LUT mapping to an And-inverter
graph using the default settings:

.. code-block:: c++

   aig_network aig = ...;
   mapping_view mapped_aig{aig};
   lut_map( mapped_aig );

Note that the AIG is wrapped into a `mapping_view` in order to equip the
network structure with the required mapping methods.

The next example is more complex.  It uses an MIG as underlying network
structure, maps for area, changes the cut size :math:`k` to 8, and also
computes the functions for the cut of each mapped node:

.. code-block:: c++

   mig_network mig = ...;
   mapped_view<mig_network, true> mapped_mig{mig};

   lut_map_params ps;
   ps.area_oriented_mapping = true;
   ps.edge_optimization = false;
   ps.remove_dominated_cuts = false;
   ps.recompute_cuts = false;
   ps.cut_enumeration_ps.cut_size = 8;
   lut_map<mapped_view<mig_network, true>, true>( mapped_mig, ps );

**Parameters and statistics**

.. doxygenstruct:: mockturtle::lut_map_params
   :members:

.. doxygenstruct:: mockturtle::lut_map_stats
   :members:

**Algorithm**

.. doxygenfunction:: mockturtle::lut_map


LUT mapping 2
-------------

Dynamic-programming based heuristic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/algorithms/lut_mapping.hpp``

LUT mapping with cut size :math:`k` partitions a logic network into mapped
nodes and unmapped nodes, where a mapped node :math:`n` is assigned some cut
:math:`(n, L)` such that the following conditions hold: i) each node that
drives a primary output is mapped, and ii) each leaf in the cut of a mapped
node is either a primary input or also mapped.  This ensures that the mapping
covers the whole logic network.  LUT mapping aims to find a good mapping with
respect to a cost function, e.g., a short critical path in the mapping or a
small number of mapped nodes.  The LUT mapping algorithm can assign a weight
to a cut for alternative cost functions.

This implementation performs ony area-oriented mapping.

The following example shows how to perform a LUT mapping to an And-inverter
graph using the default settings:

.. code-block:: c++

   aig_network aig = ...;
   mapping_view mapped_aig{aig};
   lut_mapping( mapped_aig );

Note that the AIG is wrapped into a `mapping_view` in order to equip the
network structure with the required mapping methods.

The next example is more complex.  It uses an MIG as underlying network
structure, changes the cut size :math:`k` to 8, and also computes the functions
for the cut of each mapped node:

.. code-block:: c++

   mig_network mig = ...;
   mapped_view<mig_network, true> mapped_mig{mig};

   lut_mapping_params ps;
   ps.cut_enumeration_ps.cut_size = 8;
   lut_mapping<mapped_view<mig_network, true>, true>( mapped_mig, ps );

**Parameters and statistics**

.. doxygenstruct:: mockturtle::lut_mapping_params
   :members:

.. doxygenstruct:: mockturtle::lut_mapping_stats
   :members:

**Algorithm**

.. doxygenfunction:: mockturtle::lut_mapping


SAT-based mapping
~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/algorithms/satlut_mapping.hpp``

This algorithm has a similar interface to the heuristic described above, but
uses SAT to find mappings with fewer number of cells.

**Parameters and statistics**

.. doxygenstruct:: mockturtle::satlut_mapping_params
   :members:

.. doxygenstruct:: mockturtle::satlut_mapping_stats
   :members:

**Algorithm**

.. doxygenfunction:: mockturtle::satlut_mapping(Ntk&, satlut_mapping_params const&, satlut_mapping_stats*)
.. doxygenfunction:: mockturtle::satlut_mapping(Ntk&, uint32_t, satlut_mapping_params, satlut_mapping_stats*)
