.. _network:

Network interface API
=====================

This page describes the common interfaces of a logic network data structure in
*mockturtle*. Besides those listed below, more interfaces may be extended to
a network by wrapping the network with *views* (see :ref:`views`).

.. warning::

   This part of the documentation makes use of a class called ``network``.
   This class has been created solely for the purpose of creating this
   documentation and is not meant to be used in code.  Custom network
   implementation do **not** have to derive from this class, but only need to
   ensure that, if they implement a function of the interface, it is
   implemented using the same signature.

Mandatory types and constants
-----------------------------

The interaction with a logic network data structure is performed using four
types for which no application details are assumed.  The following four types
must be defined within the network data structure.  They can be implemented as
nested types, or be exposed as type aliases.

.. doxygenclass:: mockturtle::network
   :members: base_type, node, signal, storage
   :no-link:

Further, a network must expose the following compile-time constants:

.. code-block:: c++

   static constexpr uint32_t min_fanin_size;
   static constexpr uint32_t max_fanin_size;

The struct ``is_network_type`` can be used to check at compile time whether a
given type contains all required types and constants to implement a network
type.  It should be used in the beginning of an algorithm that expects a
network type:

.. code-block:: c++

   template<typename Ntk>
   void algorithm( Ntk const& ntk )
   {
     static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
   }

Constructors and copy assignment
--------------------------------

.. doxygenclass:: mockturtle::network
   :members: network, network(storage), operator=
   :no-link:

Methods
-------

The remainder lists methods that may be implemented by a network data structure.
Algorithms can check whether a method ``method`` is implemented using the
``has_method`` struct.  As an example to check whether the method
``get_constant`` is implemented, one of the following two static assertions
can be added to the beginning of an algorithm:

.. code-block:: c++

   // variant 1
   static_assert( has_get_constant<Ntk>::value, "Ntk does not implement the get_constant method" );

   // variant 2
   static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

Duplicate network
~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: clone
   :no-link:

Primary I/O and constants
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: get_constant, create_pi, create_po, is_constant, is_pi, is_ci, constant_value
   :no-link:

Create unary functions
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: create_buf, create_not
   :no-link:

Create binary functions
~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: create_and, create_nand, create_or, create_nor, create_lt, create_le, create_gt, create_ge, create_xor, create_xnor
   :no-link:

Create ternary functions
~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: create_maj, create_ite, create_xor3
   :no-link:

Create nary functions
~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: create_nary_and, create_nary_or, create_nary_xor
   :no-link:

Create arbitrary functions
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: create_node, clone_node
   :no-link:

Restructuring
~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: substitute_node, substitute_nodes, replace_in_node, replace_in_outputs, take_out_node, is_dead
   :no-link:

Structural properties
~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: is_combinational, size, num_pis, num_pos, num_cis, num_cos, num_gates, fanin_size, fanout_size, incr_fanout_size, decr_fanout_size, depth, level, is_and, is_or, is_xor, is_maj, is_ite, is_xor3, is_nary_and, is_nary_or, is_nary_xor, is_function
   :no-link:

Functional properties
~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: node_function
   :no-link:

Nodes and signals
~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: get_node, make_signal, is_complemented, node_to_index, index_to_node, pi_at, po_at, ci_at, co_at, pi_index, po_index, ci_index, co_index
   :no-link:

Node and signal iterators
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: foreach_node, foreach_gate, foreach_pi, foreach_po, foreach_ci, foreach_co, foreach_fanin, foreach_fanout
   :no-link:

Simulate values
~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: compute
   :no-link:

Custom node values
~~~~~~~~~~~~~~~~~~

Each node can be assigned a value, which is a 32-bit unsigned integer.  The
default value is 0.  Note that all value-functions are constant, because a
change to the values is considered transparent to the network.  If a caller
passes a constant network to an algorithm, the algorithm may change the values
but cannot change the structure of the network or any other *visible* property.

.. warning::

   Values are meant to use internally in the implementation of an algorithm.
   Users of these utility function should make sure not to call other algorithms
   that may overwrite the values.  Exclusive access to temporary storage can
   only be guaranteed by using custom containers.

.. doxygenclass:: mockturtle::network
   :members: clear_values, value, set_value, incr_value, decr_value
   :no-link:

Visited flags
~~~~~~~~~~~~~

Visited flags are similar to custom node values, but are used for the specific
purpose of checking whether a node was visited in traversing algorithms.
Again, all visited-functions are constant, because a change to the visited
flags is considered transparent to the network.  If a caller passes a constant
network to an algorithm, the algorithm may change the visited flags but cannot
change the structure of the network or any other *visible* property.  The use
of traversal ids helps to use unique visited flags in multiple depending
contexts.

.. doxygenclass:: mockturtle::network
   :members: clear_visited, visited, set_visited, trav_id, incr_trav_id
   :no-link:

General methods
~~~~~~~~~~~~~~~

.. doxygenclass:: mockturtle::network
   :members: events
   :no-link:
