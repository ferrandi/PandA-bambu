Resubstitution
--------------

**Header:** ``mockturtle/algorithms/resubstitution.hpp``

Several resubstitution algorithms are implemented and can be called directly, including:

- ``default_resubstitution`` does functional reduction within a window.

- ``aig_resubstitution``, ``mig_resubstitution`` and ``xmg_resubstitution`` do window-based resubstitution in the corresponding network types.

- ``resubstitution_minmc_withDC`` minimizes multiplicative complexity in XAGs with window-based resubstitution.

- ``sim_resubstitution`` does simulation-guided resubstitution in AIGs or XAGs.


The following example shows how to resubstitute nodes in an MIG.

.. code-block:: c++

   /* derive some MIG */
   mig_network mig = ...;

   /* resubstitute nodes */
   mig_resubstitution( mig );
   mig = cleanup_dangling( mig );


Parameters and statistics
~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::resubstitution_params
   :members:

.. doxygenstruct:: mockturtle::resubstitution_stats
   :members:

.. _resubstitution_structure:

Structure
~~~~~~~~~

In addition to the example algorithms listed above, custom resubstitution algorithms can also be composed.

**Top level**

First, the top-level framework ``detail::resubstitution_impl`` is built-up with a resubstitution engine and a divisor collector.

.. doxygenclass:: mockturtle::detail::resubstitution_impl

.. doxygenfunction:: mockturtle::detail::resubstitution_impl::resubstitution_impl

**ResubEngine**

There are two resubstitution engines implemented: `window_based_resub_engine` and `simulation_based_resub_engine`.

.. doxygenclass:: mockturtle::detail::window_based_resub_engine

.. doxygenclass:: mockturtle::detail::simulation_based_resub_engine

**DivCollector**

Currently, there is only one implementation:

.. doxygenclass:: mockturtle::detail::default_divisor_collector

**Example**

The following example shows how to compose a customized resubstitution algorithm.

.. code-block:: c++

   /* derive some AIG */
   aig_network aig = ...;

   /* prepare the needed views */
   using resub_view_t = fanout_view<depth_view<aig_network>>;
   depth_view<aig_network> depth_view{aig};
   resub_view_t resub_view{depth_view};

   /* compose the resubstitution framework */
   using validator_t = circuit_validator<Ntk, bill::solvers::bsat2, false, true, false>;
   using functor_t = typename detail::sim_aig_resub_functor<resub_view_t, validator_t>;
   using engine_t = typename detail::simulation_based_resub_engine<resub_view_t, validator_t, functor_t>;
   using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, engine_t>;

   /* statistics objects */
   resubstitution_stats st;
   typename resub_impl_t::engine_st_t engine_st;
   typename resub_impl_t::collector_st_t collector_st;

   /* instantiate the framework and run it */
   resubstitution_params ps;
   resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
   p.run();
   
   /* report statistics */
   st.report();
   collector_st.report();
   engine_st.report();

Detailed statistics
~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: mockturtle::detail::window_resub_stats
   :members:

.. doxygenstruct:: mockturtle::detail::sim_resub_stats
   :members:
