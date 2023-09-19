Utility functions
-----------------

Manipulate windows with network data types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/utils/network_utils.hpp``

.. doxygenfunction:: mockturtle::clone_subnetwork( Ntk const&, std::vector<typename Ntk::node> const&, std::vector<typename Ntk::signal> const&, std::vector<typename Ntk::node> const&, SubNtk& )

.. doxygenfunction:: mockturtle::insert_ntk( Ntk&, BeginIter, EndIter, SubNtk const&, Fn&& )

Restore network and PI/PO names
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/utils/name_utils.hpp``

.. doxygenfunction:: mockturtle::restore_network_name( const NtkSrc& ntk_src, NtkDest& ntk_dest )

.. doxygenfunction:: mockturtle::restore_names( const NtkSrc& ntk_src, NtkDest& ntk_dest, node_map<signal<NtkDest>, NtkSrc>& old2new )

.. doxygenfunction:: mockturtle::restore_pio_names_by_order( const NtkSrc& ntk_src, NtkDest& ntk_dest )
