.. _cover_to_graph:

COVER to graph conversion
-------------------------

**Header:** ``mockturtle/algorithms/cover_to_graph.hpp``

This header file defines a function to convert a network of type `cover_network` into a
new graph, of type `Ntk`. The new data structure can be one of the following: AIG, XAG, MIG or XMG.
Any node of the cover network is a function defined by specifying either its onset or its offset.
In the first case, the converter function computes the balanced Sum Of Product (SOP).
In the latter case, the function computes the Product Of Sum (POS).

The following example shows how to resynthesize a COVER network into an AIG, a XAG, a MIG and a XMG.

.. code-block:: c++

   /* derive some cover network */
    const cover_network cover = ...;

   /* define the destination networks */
    aig_network aig;
    xag_network xag;
    mig_network mig;
    xmg_network xmg;

    /* inline conversion of the cover network into the desired data structure */
    convert_cover_to_graph( cover_ntk, aig );
    convert_cover_to_graph( cover_ntk, xag ); 

    /* out-of-place conversion of the cover network into the desired data structure */
    mig = convert_cover_to_graph<mig_network>( cover_ntk );
    xmg = convert_cover_to_graph<xmg_network>( cover_ntk );

.. doxygenfunction:: mockturtle::convert_cover_to_graph( Ntk&, const cover_network& )

.. doxygenfunction:: mockturtle::convert_cover_to_graph( const cover_network& )