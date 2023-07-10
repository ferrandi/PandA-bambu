AQFP Resynthesis
----------------

AQFP Resynthesis converts a given LUT network into an AQFP network and determines the gate-levels for the new network to optimize the additional buffers needed in path balancing.

**Header:** ``mockturtle/algorithms/aqfp/aqfp_resynthesis.hpp``

.. doxygenfunction:: mockturtle::aqfp_resynthesis( NtkDest& ntk_dest, NtkSrc const& ntk_src, NodeResynFn&& node_resyn_fn, FanoutResynFn&& fanout_resyn_fn, aqfp_resynthesis_params const& ps = { false }, aqfp_resynthesis_stats* pst = nullptr )

AQFP Node Resynthesis 
~~~~~~~~~~~~~~~~~~~~~

AQFP Node Resynthesis resynthesizes a given LUT as a part of an AQFP network and determine the levels for the newly created gates.

**Header:** ``mockturtle/algorithms/aqfp/aqfp_node_resyn.hpp``

.. doxygenstruct:: mockturtle::aqfp_node_resyn
   :members:

AQFP Fanout Resynthesis 
~~~~~~~~~~~~~~~~~~~~~~~

After a LUT is resynthesized using AQFP Node Resynthesis, AQFP Fanout Resynthesis will be used to determine the starting levels of the fanins of subsequent LUTs.

**Header:** ``mockturtle/algorithms/aqfp/aqfp_fanout_resyn.hpp``

.. doxygenstruct:: mockturtle::aqfp_fanout_resyn
   :members:
