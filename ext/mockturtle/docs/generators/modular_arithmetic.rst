Modular arithmetic networks
---------------------------

The file ``mockturtle/generators/modular_arithmetic.hpp`` implements several
functions to generate modular arithmetic networks.

Addition and Subtraction
~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenfunction:: mockturtle::modular_adder_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&)
.. doxygenfunction:: mockturtle::modular_adder_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)
.. doxygenfunction:: mockturtle::modular_adder_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::modular_adder
.. doxygenfunction:: mockturtle::modular_adder_hiasat_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)
.. doxygenfunction:: mockturtle::modular_adder_hiasat_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::modular_subtractor_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&)
.. doxygenfunction:: mockturtle::modular_subtractor_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)
.. doxygenfunction:: mockturtle::modular_subtractor_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::modular_subtractor

Multiplication
~~~~~~~~~~~~~~

.. doxygenfunction:: mockturtle::modular_multiplication_inplace(Ntk& ntk, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, uint64_t)
.. doxygenfunction:: mockturtle::modular_multiplication_inplace(Ntk& ntk, std::vector<signal<Ntk>>&, std::vector<signal<Ntk>> const&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::modular_multiplication
.. doxygenfunction:: mockturtle::modular_doubling_inplace(Ntk&, std::vector<signal<Ntk>>&, uint64_t)
.. doxygenfunction:: mockturtle::modular_doubling_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::modular_halving_inplace(Ntk&, std::vector<signal<Ntk>>&, uint64_t)
.. doxygenfunction:: mockturtle::modular_halving_inplace(Ntk&, std::vector<signal<Ntk>>&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::modular_constant_multiplier
.. doxygenfunction:: mockturtle::montgomery_multiplication(Ntk&, std::vector<signal<Ntk>> const&, std::vector<signal<Ntk>> const&, std::vector<bool> const&, std::vector<bool> const&)
.. doxygenfunction:: mockturtle::montgomery_multiplication(Ntk&, std::vector<signal<Ntk>> const&, std::vector<signal<Ntk>> const&, uint64_t)

Utility functions
~~~~~~~~~~~~~~~~~

.. doxygenfunction:: mockturtle::bool_vector_from_hex
