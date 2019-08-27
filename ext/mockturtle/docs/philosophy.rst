The *mockturtle* philosophy
---------------------------

.. image:: /_static/flow.svg
   :align: right

The mockturtle philosophy is based on a principle of 4 layers that depend on
each other in a linear order as depicted in the figure on the right-hand side.
The fundament, depicted by the bottom layer, is provided by the :ref:`network`.
It defines naming conventions for types and methods in classes that implement
network interfaces, some of which are mandatory while others are optional.  The
network interface API does *not* provide any implementations for a network
though.

Algorithms, the second layer, are implemented in terms of generic functions
that takes as input an instance of some hypothetical network type and require
that type to implement all mandatory and some optional interfaces.  The
algorithms do however make *no* assumption on the internal implementation of
the input network.  For instance, they make no assumption on how gates of the
network are internally represented.

The third layer consists of actual network implementations for some network
types that implement the network interface API, e.g., And-inverter graphs,
Majority-inverter graphs, or *k*-LUT networks.  Algorithms from the second
layer can be called on instances of these networks types, if they implement the
required interfaces.  Static compile time assertions are guaranteeing that
compilation succeeds only for those network implementations that do provide all
required types and methods.

Finally, in order to improve the performance some algorithmic details may be
specialized for some network types based on their internal implementation.
This can be done for each network individually, without affecting the generic
algorithm implementation nor the implementation of other network types.
