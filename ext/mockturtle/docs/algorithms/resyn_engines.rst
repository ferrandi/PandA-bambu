Logic resynthesis
-----------------

**Headers:** ``mockturtle/algorithms/resyn_engines/mig_resyn_engines.hpp``, ``mockturtle/algorithms/resyn_engines/xag_resyn_engines.hpp``

The problem of *logic resynthesis* is defined as follows:

Given a *target* function :math:`f` and a set of *divisor* functions :math:`g_1, ..., g_n`, find a *dependency function* :math:`h` such that :math:`f=h(g_1, ..., g_n)`.

Specifically, the dependency function is represented by a *dependency circuit* of a certain network type and we aim at finding a small dependency circuit.
The logic resynthesis engines can be used in resubstitution to find the replacement for the root node. Interfacing resubstitution functors (see :ref:`resubstitution_structure` of the resubstitution framework) are provided in ``mockturtle/algorithms/mig_resub.hpp`` and ``mockturtle/algorithms/sim_resub.hpp``.

Interfaces
~~~~~~~~~~

.. doxygenclass:: mockturtle::detail::resyn_functor

.. doxygenstruct:: mockturtle::mig_resyn_functor


Algorithms
~~~~~~~~~~

.. doxygenclass:: mockturtle::xag_resyn_engine

.. doxygenclass:: mockturtle::mig_resyn_engine

.. doxygenclass:: mockturtle::mig_resyn_engine_bottom_up

.. doxygenclass:: mockturtle::mig_resyn_engine_akers
