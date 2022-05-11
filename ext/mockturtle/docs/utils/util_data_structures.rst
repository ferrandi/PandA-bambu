Utility data structures
-----------------------

Truth table cache
~~~~~~~~~~~~~~~~~

**Header:** ``mockturtle/utils/truth_table_cache.hpp``

.. doc_overview_table:: classmockturtle_1_1truth__table__cache
   :column: Method

   truth_table_cache
   insert
   operator[]
   size

.. doxygenclass:: mockturtle::truth_table_cache
   :members:

Node map
~~~~~~~~

**Header:** ``mockturtle/utils/node_map.hpp``

.. doc_overview_table:: classmockturtle_1_1node__map
   :column: Method

   node_map
   operator[]
   reset

.. doxygenclass:: mockturtle::node_map
   :members:

.. doxygenfunction:: mockturtle::initialize_copy_network

Cuts
~~~~

**Header:** ``mockturtle/utils/cuts.hpp``

.. doc_overview_table:: classmockturtle_1_1cut
   :column: Method

   operator=
   set_leaves
   signature
   size
   begin
   end
   operator->
   data
   subsumes
   merge

.. doxygenclass:: mockturtle::cut
   :members:

Cut sets
~~~~~~~~

**Header:** ``mockturtle/utils/cuts.hpp``

.. doc_overview_table:: classmockturtle_1_1cut__set
   :column: Method

   cut_set
   clear
   add_cut
   is_subsumed
   insert
   begin
   end
   size
   operator[]
   best
   update_best
   limit
   operator<<

.. doxygenclass:: mockturtle::cut_set
   :members:

Stopwatch
~~~~~~~~~

**Header:** ``mockturtle/utils/stopwatch.hpp``

.. doc_overview_table:: classmockturtle_1_1stopwatch
   :column: Method

   stopwatch
   ~stopwatch

.. doxygenclass:: mockturtle::stopwatch
   :members:

.. doxygenfunction:: mockturtle::call_with_stopwatch

.. doxygenfunction:: mockturtle::make_with_stopwatch

.. doxygenfunction:: mockturtle::to_seconds

Progress bar
~~~~~~~~~~~~

**Header:** ``mockturtle/utils/progress_bar.hpp``

.. doc_overview_table:: classmockturtle_1_1progress__bar
   :column: Method

   progress_bar
   ~progress_bar
   operator()
   done

.. doxygenclass:: mockturtle::progress_bar
   :members:
