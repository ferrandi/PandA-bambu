Debugging toolset
=================

When encountering bugs --- either a segmentation fault, or some algorithm does not
operate as expected --- some of the tools described in this section might be helpful
in figuring out the source of error.

Testcase minimizer
------------------

**Header:** ``mockturtle/algorithms/testcase_minimizer.hpp``

Often, the testcase that triggers the bug is big. The testcase minimizer helps reducing
the size of the testcase as much as possible, while keeping the buggy behavior triggered.
A minimized testcase not only facilitates debugging, but also enhances communication
between developers if the original testcase cannot be disclosed.

.. doxygenstruct:: mockturtle::testcase_minimizer_params
   :members:

.. doxygenclass:: mockturtle::testcase_minimizer

Fuzz testing
------------

**Header:** ``mockturtle/algorithms/network_fuzz_tester.hpp``

If minimizing the testcase is not successful, is too slow, or there is not an initial
testcase to start with, fuzz testing can help generating small testcases that triggers
unwanted behaviors.

.. doxygenstruct:: mockturtle::fuzz_tester_params
   :members:

.. doxygenclass:: mockturtle::network_fuzz_tester

Debugging utilities
-------------------

**Header:** ``mockturtle/utils/debugging_utils.hpp``

Some utility functions are provided in this header file. They can be added as
assertions in algorithms to identify abnormal network operations, or be used
as the checks in testcase minimizer or fuzz testing.

.. doxygenfunction:: mockturtle::network_is_acyclic

.. doxygenfunction:: mockturtle::count_dead_nodes

.. doxygenfunction:: mockturtle::count_dangling_roots

.. doxygenfunction:: mockturtle::count_reachable_dead_nodes

.. doxygenfunction:: mockturtle::count_reachable_dead_nodes_from_node

.. doxygenfunction:: mockturtle::count_nodes_with_dead_fanins

.. doxygenfunction:: mockturtle::check_network_levels

.. doxygenfunction:: mockturtle::check_fanouts

.. doxygenfunction:: mockturtle::check_window_equivalence

Visualization
-------------

Drawing a figure
~~~~~~~~~~~~~~~~

When the testcase is small enough, it becomes possible to visualize the network.
mockturtle supports writing out a network into the DOT format, which can then be
visualized using Graphviz. (:ref:`write_dot`)

Printing method
~~~~~~~~~~~~~~~

**Header:** ``mockturtle/utils/debugging_utils.hpp``

.. doxygenfunction:: mockturtle::print


Time machine
------------

Coming soon...


