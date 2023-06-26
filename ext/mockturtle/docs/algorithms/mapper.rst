Technology mapping and network conversion
-----------------------------------------

**Header:** ``mockturtle/algorithms/mapper.hpp``

A versatile mapper that supports technology mapping and graph mapping
(optimized network conversion). The mapper is independent of the
underlying graph representation. Hence, it supports generic subject
graph representations (e.g., AIG, and MIG) and a generic target
representation (e.g. cell library, XAG, XMG). The mapper aims at finding a
good mapping with respect to delay, area, and switching power.

The mapper uses a library (hash table) to facilitate Boolean matching.
For technology mapping, it needs `tech_library` while for graph mapping
it needs `exact_library`. For technology mapping, the generation of both NP- and
P-configurations of gates are supported. Generally, it is convenient to use
NP-configurations for small or medium size cell libraries. For bigger libraries,
P-configurations should perform better. You can test both the configurations to
see which one has the best run time. For graph mapping, NPN classification
is used instead.

The following example shows how to perform delay-oriented technology mapping
from an and-inverter graph using the default settings:

.. code-block:: c++

   aig_network aig = ...;

   /* read cell library in genlib format */
   std::vector<gate> gates;
   std::ifstream in( ... );
   lorina::read_genlib( in, genlib_reader( gates ) )
   tech_library tech_lib( gates );

   /* perform technology mapping */
   binding_view<klut_network> res = map( aig, tech_lib );

The mapped network is returned as a `binding_view` that extends a k-LUT network.
Each k-LUT abstracts a cell and the view contains the binding information.

The next example performs area-oriented graph mapping from AIG to MIG
using a NPN resynthesis database of structures:

.. code-block:: c++

   aig_network aig = ...;
   
   /* load the npn database in the library */
   mig_npn_resynthesis resyn{ true };
   exact_library<mig_network, mig_npn_resynthesis> exact_lib( resyn );

   /* perform graph mapping */
   map_params ps;
   ps.skip_delay_round = true;
   ps.required_time = std::numeric_limits<double>::max();
   mig_network res = map( aig, exact_lib, ps );

For graph mapping, we suggest reading the network directly in the
target graph representation if possible (e.g. read an AIG as a MIG)
since the mapping often leads to better results in this setting.

For technology mapping of sequential networks, a dedicated command
`seq_map` should be called. Only in case of graph mapping, the
command `map` can be used on sequential networks.

The following example shows how to perform delay-oriented technology
mapping from a sequential and-inverter graph:

.. code-block:: c++

   sequential<aig_network> aig = ...;

   /* read cell library in genlib format */
   std::vector<gate> gates;
   std::ifstream in( ... );
   lorina::read_genlib( in, genlib_reader( gates ) )
   tech_library tech_lib( gates );

   /* perform technology mapping */
   using res_t = binding_view<sequential<klut_network>>;
   res_t res = seq_map( aig, tech_lib );

The next example performs area-oriented graph mapping from a 
sequential AIG to a sequential MIG using a NPN resynthesis
database of structures:

.. code-block:: c++

   sequential<aig_network> aig = ...;
   
   /* load the npn database in the library */
   mig_npn_resynthesis resyn{ true };
   exact_library<sequential<mig_network>, mig_npn_resynthesis> exact_lib( resyn );

   /* perform graph mapping */
   map_params ps;
   ps.skip_delay_round = true;
   ps.required_time = std::numeric_limits<double>::max();
   sequential<mig_network> res = map( aig, exact_lib, ps );

As a default setting, cut enumeration minimizes the truth tables.
This helps improving the results but slows down the computation.
We suggest to keep it always true. Anyhow, for a faster mapping,
set the truth table minimization parameter to false.
The maximum number of cuts stored for each node is limited to 49.
To increase this limit, change `max_cut_num` in `fast_network_cuts`.

**Parameters and statistics**

.. doxygenstruct:: mockturtle::map_params
   :members:

.. doxygenstruct:: mockturtle::map_stats
   :members:

**Algorithm**

.. doxygenfunction:: mockturtle::map(Ntk const&, tech_library<NInputs, Configuration> const&, map_params const&, map_stats*)
.. doxygenfunction:: mockturtle::map(Ntk&, exact_library<NtkDest, RewritingFn, NInputs> const&, map_params const&, map_stats*)
