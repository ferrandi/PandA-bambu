.. _views:

Views
-----

Views can modify the implementation of a network interface by

1. adding interface methods that are not supported by the implementation,
2. changing the implementation of interface methods, and
3. deleting interface methods.

Views implement the network interface and can be passed like a network to an
algorithm.  Several views are implemented in mockturtle.

`topo_view`: Ensure topological order
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/topo_view.hpp``

.. doxygenclass:: mockturtle::topo_view
   :members:

`depth_view`: Compute levels and depth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/depth_view.hpp``

.. doxygenclass:: mockturtle::depth_view
   :members:

`rank_view`: Order nodes within each level
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/rank_view.hpp``

.. doxygenclass:: mockturtle::rank_view
   :members:

`mapping_view`: Add mapping interface methods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/mapping_view.hpp``

.. doxygenclass:: mockturtle::mapping_view
   :members:

`cut_view`: Network view on a single rooted cut
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/cut_view.hpp``

.. doxygenclass:: mockturtle::cut_view
   :members:

`mffc_view`: Network view on a (M)FFC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/mffc_view.hpp``

.. doxygenclass:: mockturtle::mffc_view
   :members:

`immutable_view`: Prevent network changes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/immutable_view.hpp``

.. doxygenclass:: mockturtle::immutable_view
   :members:

`fanout_view`: Compute fanout
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/fanout_view.hpp``

.. doxygenclass:: mockturtle::fanout_view
   :members:

`window_view`: Network view on a window
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/window_view.hpp``

.. doxygenclass:: mockturtle::window_view
   :members:

`binding_view`: Add bindings to a technology library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/binding_view.hpp``

.. doxygenclass:: mockturtle::binding_view
   :members:

`names_view`: Assign names to signals and outputs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/choice_view.hpp``

.. doxygenclass:: mockturtle::choice_view
   :members:

`choice_view`: Implements choices in networks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/names_view.hpp``

.. doxygenclass:: mockturtle::names_view
   :members:

`cnf_view`: Creates a CNF while creating a network
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/cnf_view.hpp``

.. doxygenclass:: mockturtle::cnf_view
   :members:

`color_view`: Manages traversal IDs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/color_view.hpp``

.. doxygenclass:: mockturtle::color_view
   :members:

.. doxygenclass:: mockturtle::out_of_place_color_view
   :members:

`cost_view`: Manages global cost and maintains context
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/cost_view.hpp``

.. doxygenclass:: mockturtle::cost_view
   :members:

`dont_care_view`: Manages external don't care information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/views/dont_care_view.hpp``

.. doxygenclass:: mockturtle::dont_care_view
