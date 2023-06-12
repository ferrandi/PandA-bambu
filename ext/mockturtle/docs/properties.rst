Properties
==========

MIG-based costs
---------------

In header ``mockturtle/properties/migcost.hpp`` functions are defined that can
be used to compute costs relevant in majority-based emerging technologies.

.. doxygenfunction:: mockturtle::num_inverters

.. doxygenfunction:: mockturtle::num_dangling_inputs



Multiplicative complexity costs
-------------------------------

In header ``mockturtle/properties/mccost.hpp`` functions are defined that can
be used to compute costs based on the multiplicative complexity of the network.

.. doxygenfunction:: mockturtle::multiplicative_complexity

.. doxygenfunction:: mockturtle::multiplicative_complexity_depth



Factored form literals costs
----------------------------

In header ``mockturtle/properties/litcost.hpp`` functions are defined that can
be used to compute costs based on the factored form literal count of Boolean 
functions.

.. doxygenfunction:: mockturtle::factored_literal_cost(std::vector<kitty::cube> const&, uint32_t)

.. doxygenfunction:: mockturtle::factored_literal_cost(kitty::dynamic_truth_table const&, bool)

.. doxygenfunction:: mockturtle::factored_literal_cost(kitty::dynamic_truth_table const&, kitty::dynamic_truth_table const&, bool)